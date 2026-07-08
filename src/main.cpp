#include <algorithm>
#include <cassert>
#include <curl/curl.h>
#include <fstream>
#include "haversine.hpp"
#include <iostream>
#include <numeric>
#include <nlohmann/json.hpp>
#include <random>
#include <vector>
#include <string>

using JSON = nlohmann::json;
using State = std::vector<int>;

struct City {
    double lat;
    double lng;
};

const State &tournament_select(const std::vector<State> &pop, const std::vector<City> &cities,
                        std::mt19937 &g, int k = 5);
double fitness(const State &state, const std::vector<City> &cities);
double tour_dist(const State &state, const std::vector<City> &cities);
State random_state(const int N);
std::string trim(const std::string &value);
std::string read_env_value(const std::string &key);
size_t receive_data(void *contents, size_t size, size_t count, void *userp);

int main() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        // HTTP request failed
        std::cerr << "Failed to initialize CURL\n";
        return 1;
    }

    std::string response;
    
    const std::string GEONAMES_USERNAME = read_env_value("GEONAMES_USERNAME");
    if (GEONAMES_USERNAME.empty()) {
        curl_easy_cleanup(curl);
        std::cerr << "GEONAMES_USERNAME environment variable is not set\n";
        return 1;
    }

    const std::string URL = "https://secure.geonames.org/searchJSON"
                            "?country=IL&featureClass=P&maxRows=50&username=" + GEONAMES_USERNAME;

    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, static_cast<long>(1));

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, static_cast<long>(10));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(15));

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        curl_easy_cleanup(curl);
        std::cerr << "Failed to perform CURL request\n";
        return 1;
    }

    long http_status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
    if (http_status != 200) {
        curl_easy_cleanup(curl);
        std::cerr << "HTTP request failed with status code: " << http_status << '\n';
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
State random_state(const int N=50) {
    // Create a state with N ordered cities (0 to N-1)
    State state(N);
    std::iota(state.begin(), state.end(), 0);

    // Shuffle the ordered state to create a random state
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(state.begin(), state.end(), g);

    return state;
}

// Returns the total distance of a tour represented by a state
double tour_dist(const State &state, const std::vector<City> &cities) {
    double total_dist = 0.0;
    for (int i = 0; i < state.size(); i++) {
        const City &to_city = cities[state[i]];
        const City &from_city = cities[state[i + 1 % state.size()]];
        total_dist += haversine_dist(from_city.lat, from_city.lng,
                                       to_city.lat, to_city.lng);
    }
    return total_dist;
}

// Converts state's tour's distance into a fitness score
double fitness(const State &state, const std::vector<City> &cities) {
    double dist = tour_dist(state, cities);

    // Fitness score is inversely proportional to distance
    // Add a small epsilon to avoid division by 0 (0 distance edge case)
    return 1.0 / (dist + 1e-9);
}

const State &tournament_select(const std::vector<State> &pop, const std::vector<City> &cities,
                        std::mt19937 &g, int k) {
    assert(!pop.empty());

    std::uniform_int_distribution<size_t> distribution(0, pop.size() - 1);
    size_t best_candidate_idx = distribution(g);
    double best_score = fitness(pop[best_candidate_idx], cities);

    for (int i = 0; i < k - 1; i++) {
        const size_t candidate_idx = distribution(g);
        const double candidate_score = fitness(pop[candidate_idx], cities);

        if (best_score < candidate_score) {
            // Current candidate becomes the best candidate
            best_candidate_idx = candidate_idx;
            best_score = candidate_score;
        }
    }
    return pop[best_candidate_idx];  // Best candidate from tournament
}

//
std::string trim(const std::string &value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

/* 
 * Reads value of a given key from a .env file
 * Must cite AI-generated code: logic for reading from .env file 
 * generated by GitHub Copilot (and Claude), modified by dev
 */
std::string read_env_value(const std::string &key) {
    std::ifstream env_file(".env");
    if (!env_file.is_open()) {
        return "";
    }

    std::string line;
    while (std::getline(env_file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        std::string current_key = trim(line.substr(0, separator));
        std::string current_value = trim(line.substr(separator + 1));

        if (current_value.size() >= 2 && current_value.front() == '"' 
                                      && current_value.back() == '"') {
            current_value = current_value.substr(1, current_value.size() - 2);
        }

        if (current_key == key) {
            return current_value;
        }
    }

    return "";
}

//
size_t receive_data(void *contents, size_t size, size_t count, void *userp) {
    size_t total_size = size * count;
    std::string *res = static_cast<std::string *>(userp);
    res->append(static_cast<char *>(contents),  total_size);

    return total_size;
}

