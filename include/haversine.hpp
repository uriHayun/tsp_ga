#ifndef HAVERSINE_HPP
#define HAVERSINE_HPP

#include <cmath>

/* 
 * Returns (air) distance in km between 2 points using Haversine formula
 * Note: < ~0.5% error (typically less), not worth using more precise formulas (e.g. Vincenty's)
 */
inline double haversine_dist(double lat1, double lng1, double lat2, double lng2) {
    // Convert coordinates (in degrees) to radians
    auto degrees_to_radians = [](double deg) { return deg * M_PI / 180.0; };

    lat1 = degrees_to_radians(lat1);
    lng1 = degrees_to_radians(lng1);
    lat2 = degrees_to_radians(lat2);
    lng2 = degrees_to_radians(lng2);

    // Differences in latitude/longitude between the 2 points (radians)
    double dlat = lat2 - lat1;
    double dlng = lng2 - lng1;

    // Compute haversine of the central angle in order to compute angular distance ("c") later
    double a = std::pow(std::sin(dlat / 2.0), 2)
             + std::cos(lat1) * std::cos(lat2) * std::pow(std::sin(dlng / 2.0), 2);

    // Compute angular distance (radians)
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

    constexpr int earth_radius_km = 6371;
    return earth_radius_km * c;  // Air distance (km)
}

#endif