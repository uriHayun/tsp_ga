#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

using JSON = nlohmann::json;

struct City {
    double lat;
    double lng;
};

size_t receive_data(void *contents, size_t size, size_t count, void *userp);

int main() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        // HTTP request failed
        return 1;
    }

    std::string response;

    const std::string URL = "https://secure.geonames.org/searchJSON"
                      "?country=IL&featureClass=P&maxRows=50&username=urih";

    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, static_cast<long>(1));

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, static_cast<long>(10));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(15));

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        curl_easy_cleanup(curl);
        return 1;
    }

    long http_status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
    if (http_status != 200) {
        curl_easy_cleanup(curl);
        return 1;
    }

    if (response.empty()) {
        curl_easy_cleanup(curl);
        return 1;
    }

    curl_easy_cleanup(curl);

    try {
        JSON data = JSON::parse(response);
        if (!data.contains("geonames") || !data["geonames"].is_array()) {
            return 1;
        }

        std::vector<City> cities;

        for (const auto &city : data["geonames"]) {
            cities.push_back({
                city["lat"],
                city["lng"]
            });
        }
    }
    catch (const JSON::parse_error &e) {
        return 1;
    } 
    catch (JSON::exception &e) {
        return 1;
    }

    return 0;
}

size_t receive_data(void *contents, size_t size, size_t count, void *userp) {
    size_t total_size = size * count;
    std::string *res = static_cast<std::string *>(userp);
    res->append(static_cast<char *>(contents),  total_size);

    return total_size;
}