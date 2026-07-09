#include "haversine.hpp"
#include "tour.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <numeric>
#include <nlohmann/json.hpp>
#include <random>
#include <vector>
#include <string>

using JSON = nlohmann::json;

struct City {
    double lat;
    double lng;
};

const Tour &tournamentSelect(const std::vector<Tour> &pop, const std::vector<City> &cities,
                        std::mt19937 &g, int k = 5);
double fitness(const Tour &tour, const std::vector<City> &cities);
double tourDist(const Tour &tour, const std::vector<City> &cities);
Tour randomTour(const int N = 50);
std::string trim(const std::string &value);
std::string readEnvValue(const std::string &key);
std::size_t receiveData(void *contents, std::size_t size, std::size_t count, void *userp);

int main() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        // HTTP request failed
        std::cerr << "Failed to initialize CURL\n";
        return 1;
    }

    std::string response;
    
    const std::string GEONAMES_USERNAME = readEnvValue("GEONAMES_USERNAME");
    if (GEONAMES_USERNAME.empty()) {
        curl_easy_cleanup(curl);
        std::cerr << "GEONAMES_USERNAME environment variable is not set\n";
        return 1;
    }

    const std::string URL = "https://secure.geonames.org/searchJSON"
                            "?country=IL&featureClass=P&maxRows=50&username=" + GEONAMES_USERNAME;

    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receiveData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    // Follow HTTP redirects (301, 302) automatically
    // libcurl excpects option values to be type long
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Prevent the request from hanging indefinitely by limiting the connection and total timeouts
    // libcurl excpects option values to be type long
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        curl_easy_cleanup(curl);
        std::cerr << "Failed to perform CURL request\n";
        return 1;
    }

    long httpStatus = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
    if (httpStatus != 200) {
        curl_easy_cleanup(curl);
        std::cerr << "HTTP request failed with status code: " << httpStatus << '\n';
        return 1;
    }

    if (response.empty()) {
        curl_easy_cleanup(curl);
        std::cerr << "Received empty response\n";
        return 1;
    }

    curl_easy_cleanup(curl);

    try {
        JSON data = JSON::parse(response);
        if (!data.contains("geonames") || !data["geonames"].is_array()) {
            std::cerr << "Invalid response format: 'geonames' key not found or is not an array\n";
            return 1;
        }

        std::vector<City> cities;

        for (const auto &city : data["geonames"]) {
            cities.push_back({
                std::stod(city["lat"].get<std::string>()),
                std::stod(city["lng"].get<std::string>())
            });
        }

        for (const auto &city : cities) {
            std::cout << "City: lat=" << city.lat << ", lng=" << city.lng << '\n';
        }
    }
    catch (const JSON::parse_error &e) {
        std::cerr << "Failed to parse JSON response: " << e.what() << '\n';
        return 1;
    } 
    catch (JSON::exception &e) {
        std::cerr << "JSON exception occurred: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

// Returns a random unsorted sequence of integers 0 to N-1
Tour randomTour(const int N) {
    // Create a state with N (50) ordered cities (0 to N-1)
    Tour tour(N);
    std::iota(tour.begin(), tour.end(), 0);

    // Shuffle the ordered state to create a random state
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(tour.begin(), tour.end(), g);

    return tour;
}

// Returns the total distance of a tour represented by a state
double tourDist(const Tour &tour, const std::vector<City> &cities) {
    double totalDist = 0.0;
    for (int i = 0; i < static_cast<int>(tour.size()); i++) {
        const City &toCity = cities[tour[i]];
        
        // Wrap around to first using after the last city using modulo operator
        const City &fromCity = cities[tour[(i + 1) % tour.size()]];
        totalDist += haversine_dist(fromCity.lat, fromCity.lng,
                                       toCity.lat, toCity.lng);
    }
    return totalDist;
}

// Converts tour's distance into a fitness score
double fitness(const Tour &tour, const std::vector<City> &cities) {
    double dist = tourDist(tour, cities);

    // Fitness score is inversely proportional to distance
    // Add a small epsilon to avoid division by 0 (0 distance edge case)
    return 1.0 / (dist + 1e-9);
}

const Tour &tournamentSelect(const std::vector<Tour> &pop, const std::vector<City> &cities,
                        std::mt19937 &g, int k) {
    assert(!pop.empty());

    std::uniform_int_distribution<std::size_t> distribution(0, pop.size() - 1);
    std::size_t bestCandidateIdx = distribution(g);
    double bestScore = fitness(pop[bestCandidateIdx], cities);

    for (int i = 0; i < k - 1; i++) {
        const std::size_t candidateIdx = distribution(g);
        const double candidateScore = fitness(pop[candidateIdx], cities);

        if (bestScore < candidateScore) {
            // Current candidate becomes the best candidate
            bestCandidateIdx = candidateIdx;
            bestScore = candidateScore;
        }
    }
    return pop[bestCandidateIdx];  // Best candidate from tournament
}

// Removes leading and trailing whitespace from a string
std::string trim(const std::string &value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

/* 
 * Reads the value associated with a key from a local .env file
 *
 * Must cite AI-generated code: logic for reading from .env file 
 * generated by GitHub Copilot (and Claude), modified by dev
 */
std::string readEnvValue(const std::string &key) {
    std::ifstream envFile(".env");
    if (!envFile.is_open()) {
        return "";
    }

    std::string line;
    while (std::getline(envFile, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        std::string currKey = trim(line.substr(0, separator));
        std::string currValue = trim(line.substr(separator + 1));

        if (currValue.size() >= 2 && currValue.front() == '"' 
                                      && currValue.back() == '"') {
            currValue = currValue.substr(1, currValue.size() - 2);
        }

        if (currKey == key) {
            return currValue;
        }
    }

    return "";
}

// libcurl callback to append the HTTP received response data to a string
// Note: libcurl is a C library, so this callback uses raw pointers instead of C++ references
std::size_t receiveData(void *contents, std::size_t size, std::size_t count, void *userp) {
    std::size_t totalSize = size * count;
    std::string *res = static_cast<std::string *>(userp);
    res->append(static_cast<char *>(contents),  totalSize);

    return totalSize;
}

