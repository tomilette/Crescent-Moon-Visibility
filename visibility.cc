function calculate(
    latitude, longitude, altitude, base_time,
    details = { value: null },
    ignore_besttime = false,
    draw_moon_line = { value: null },
    result_time = { value: null },
    q_value = { value: null }
) {
    let time = Astronomy_AddDays(base_time, -longitude / 360);
    let observer = { latitude: latitude, longitude: longitude, height: altitude };

    let direction = evening ? DIRECTION_SET : DIRECTION_RISE;
    let sunset_sunrise = Astronomy_SearchRiseSet(BODY_SUN, observer, direction, time, 1);
    let moonset_moonrise = Astronomy_SearchRiseSet(BODY_MOON, observer, direction, time, 1);
    if (sunset_sunrise.status != ASTRO_SUCCESS || moonset_moonrise.status != ASTRO_SUCCESS) return 'H'; // No sun{set,rise} or moon{set,rise}
    let lag_time = (moonset_moonrise.time.ut - sunset_sunrise.time.ut) * (evening ? 1 : -1);
    if (details) { details.lag_time = lag_time; details.moonset_moonrise = moonset_moonrise.time; details.sunset_sunrise = sunset_sunrise.time; }
    let best_time = (lag_time < 0 || ignore_besttime)
                           ? sunset_sunrise.time
                           : Astronomy_AddDays(sunset_sunrise.time, lag_time * 4 / 9 * (evening ? 1 : -1));
    if (result_time) result_time.value = best_time.ut;
    let new_moon_prev = Astronomy_SearchMoonPhase(0, sunset_sunrise.time, -35).time;
    let new_moon_next = Astronomy_SearchMoonPhase(0, sunset_sunrise.time, +35).time;
    let new_moon_nearest = (sunset_sunrise.time.ut - new_moon_prev.ut) <= (new_moon_next.ut - sunset_sunrise.time.ut)
        ? new_moon_prev : new_moon_next;
    if (details) { details.new_moon_prev = new_moon_prev; details.new_moon_next = new_moon_next; }
    if (draw_moon_line) draw_moon_line.value = ((int) round((best_time.ut - new_moon_nearest.ut) * 24 * 20) % 20) == 0;
    if (details) {
        details.moon_age_prev = best_time.ut - new_moon_prev.ut;
        details.moon_age_next = best_time.ut - new_moon_next.ut;
    }
    let before_new_moon = (sunset_sunrise.time.ut - new_moon_nearest.ut) * (evening ? 1 : -1) < 0;
    if (lag_time < 0 && before_new_moon) return 'J'; // Checks both of the conditions on the two below lines, shows a mixed color
    if (lag_time < 0) return 'I'; // Moonset before sunset / Moonrise after sunrise
    if (before_new_moon) return 'G'; // Sunset is before new moon / Sunrise is after new moon

    let sun_equator = Astronomy_Equator(BODY_SUN, best_time, observer, EQUATOR_OF_DATE, ABERRATION);
    let sun_horizon = Astronomy_Horizon(best_time, observer, sun_equator.ra, sun_equator.dec, REFRACTION_NONE);
    let moon_equator = Astronomy_Equator(BODY_MOON, best_time, observer, EQUATOR_OF_DATE, ABERRATION);
    let moon_horizon = Astronomy_Horizon(best_time, observer, moon_equator.ra, moon_equator.dec, REFRACTION_NONE);
    let liberation = Astronomy_Libration(best_time);

    let SD = liberation.diam_deg * 60 / 2; // Semi-diameter of the Moon in arcminutes, geocentric
    let lunar_parallax = SD / 0.27245; // In arcminutes
    // As SD_topo should be in arcminutes as SD, but moon_alt and lunar_parallax are in degrees, it is divided by 60.
    let SD_topo = SD * (1 + Math.sin(moon_horizon.altitude * DEG2RAD) * Math.sin(lunar_parallax / 60 * DEG2RAD));

    let ARCL = yallop
        ? Astronomy_Elongation(BODY_MOON, best_time).elongation // Geocentric elongation in Yallop
        : Astronomy_AngleBetween(sun_equator.vec, moon_equator.vec).angle; // Topocentric elongation in Odeh

    let DAZ = sun_horizon.azimuth - moon_horizon.azimuth;
    let ARCV;
    if (yallop) {
        let geomoon = Astronomy_GeoVector(BODY_MOON, best_time, ABERRATION);
        let geosun = Astronomy_GeoVector(BODY_SUN, best_time, ABERRATION);
        let rot = Astronomy_Rotation_EQJ_EQD(best_time);
        let rotmoon = Astronomy_RotateVector(rot, geomoon);
        let rotsun  = Astronomy_RotateVector(rot, geosun);
        let meq = Astronomy_EquatorFromVector(rotmoon);
        let seq = Astronomy_EquatorFromVector(rotsun);
        let mhor = Astronomy_Horizon(best_time, observer, meq.ra, meq.dec, REFRACTION_NONE);
        let shor = Astronomy_Horizon(best_time, observer, seq.ra, seq.dec, REFRACTION_NONE);
        ARCV = mhor.altitude - shor.altitude;
    } else { // Odeh
        let COSARCV = Math.cos(ARCL * DEG2RAD) / Math.cos(DAZ * DEG2RAD);
        if      (COSARCV < -1) COSARCV = -1;
        else if (COSARCV > +1) COSARCV = +1;
        ARCV = Math.acos(COSARCV) * RAD2DEG;
    }
    let W_topo = SD_topo * (1 - Math.cos(ARCL * DEG2RAD)); // In arcminutes

    let result = ' ';
    let value;
    if (yallop) {
        value = (ARCV - (11.8371 - 6.3226 * W_topo + .7319 * Math.pow(W_topo, 2) - .1018 * Math.pow(W_topo, 3))) / 10;
        if      (value > +.216) result = 'A'; // Crescent easily visible
        else if (value > -.014) result = 'B'; // Crescent visible under perfect conditions
        else if (value > -.160) result = 'C'; // May need optical aid to find crescent
        else if (value > -.232) result = 'D'; // Will need optical aid to find crescent
        else if (value > -.293) result = 'E'; // Crescent not visible with telescope
        else result = 'F';
    } else { // Odeh
        value = ARCV - (7.1651 - 6.3226 * W_topo + .7319 * Math.pow(W_topo, 2) - .1018 * Math.pow(W_topo, 3));
        if      (value >= 5.65) result = 'A'; // Crescent is visible by naked eye
        else if (value >= 2.00) result = 'C'; // Crescent is visible by optical aid
        else if (value >= -.96) result = 'E'; // Crescent is visible only by optical aid
        else result = 'F';
    }
    if (q_value) q_value.value = value;

    if (details) {
        details.best_time = best_time;
        details.sd = SD; details.lunar_parallax = lunar_parallax; details.arcl = ARCL; details.arcv = ARCV;
        details.daz = DAZ; details.w_topo = W_topo; details.sd_topo = SD_topo; details.value = value;
        details.moon_azimuth = moon_horizon.azimuth, details.moon_altitude = moon_horizon.altitude;
        details.moon_ra = moon_horizon.ra; details.moon_dec = moon_horizon.dec;
        details.sun_azimuth = sun_horizon.azimuth; details.sun_altitude = sun_horizon.altitude;
        details.sun_ra = sun_horizon.ra; details.sun_dec = sun_horizon.dec;
    }

    return result;
}
