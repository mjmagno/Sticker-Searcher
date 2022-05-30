//
//  StickerApplication.cpp
//  MoodMeAssessment
//
//  Created by Miguel Magno on 5/29/22.
//  Copyright © 2022 Miguel Magno. All rights reserved.
//

#include <stdio.h>
#include <iostream>
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <vector>
#include <nlohmann/json.hpp>


using asio::ip::tcp;
using json = nlohmann::json;

void welcomeMessage() {
    std::cout << "Hello! Please enter \"search <criteria>\" to get started. This app will return stickers from giphy.com corresponding to your query. Enter \"help\" to get a full list of commands." << std::endl;
}

void helpMessage() {
    std::cout << "Enter \"search <criteria>\" to return the urls of the first 10 stickers from giphy.com." << std::endl << "Enter \"next\" to return the urls of the next 10 stickers from your most recent search." << std::endl << "Enter \"cancel\" to clear your current search criteria and results." << std::endl << "Enter \"g\",\"pg\",\"pg13\",or \"r\" to return the number of stickers in each category that have been found, and optionally, their urls." << std::endl << "Enter \"end\" to stop the program." << std::endl;
    
}
class StickerSearch {
public:
    StickerSearch() : count(0), currOffset(0) {
    
    };
    //Returns a string that contains all of the data returned from giphy.com's search REST endpoint
    std::string getData() {
        if (criteria == "") {
            std::cout << "No criteria set. Please search for something." << std::endl;
            return "";
        }
        asio::io_context io_context;
        

        tcp::resolver resolver(io_context);
        tcp::resolver::query query("api.giphy.com","http");
        tcp::resolver::iterator iterator = resolver.resolve(query);
        tcp::socket socket(io_context);
        asio::connect(socket, iterator);
        asio::streambuf request;
        std::ostream request_stream(&request);
        
        //Sends a GET request to giphy with the user's search criteria
        //excludes the stickers already found
        std::string req = "GET /v1/stickers/search?api_key=sO8yzFHXqrPZx8slBGQosmSzYNfyfh1Q&q=" + criteria + "&limit=10&offset=" + std::to_string(currOffset) + "&rating=r&lang=en HTTP/1.0\r\n";
        request_stream << req;
        request_stream << "Host: " << "api.giphy.com" << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";
        asio::write(socket, request);
        
        // Reads the response status line
        asio::streambuf response;
        asio::read_until(socket, response, "\r\n");
        std::istream response_stream(&response);
        std::string http_version, status_message;
        unsigned int status_code;
        std::getline(response_stream >> http_version >> status_code, status_message);
        
        if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
            std::cout << "Invalid response\n";
            return 0;
        }
        if (status_code != 200) {
            std::cout << "Response returned with status code " << status_code << "\n";
            return 0;
        }
        
        // Reads and processes the response headers
        asio::read_until(socket, response, "\r\n\r\n");
        std::string garb;
        while (std::getline(response_stream, garb) && garb != "\r");

        
        //Reads the sticker data into line
        asio::error_code error;
        asio::read_until(socket, response, "\"meta\"",error);
        std::string line;
        std::getline(response_stream,line);
        return line;
    }
    
    //Parses the json data from giphy into a usable format
    //Populates search vectors with the resulting urls
    void populateResults(std::string line){
        if (line == ""){
            return;
        }
        //Removes any unnecessary informaton from json data for easy access
        json::parser_callback_t cb = [](int depth, json::parse_event_t event, json & parsed)
        {
            if (event == json::parse_event_t::key and parsed == json("images"))
            {
                return false;
            }
            else if (event == json::parse_event_t::key and parsed == json("analytics")){
                return false;
            }
            else if (event == json::parse_event_t::key and parsed == json("analytics_response_payload")){
                return false;
            }
            else
            {
                return true;
            }
        };
        
        // parse (with callback) and serialize JSON
        json j_filtered = json::parse(line, cb);
        count = j_filtered["pagination"]["total_count"];
        
        if (currOffset >= count) {
            std::cout << "No data" << std::endl;
            return;
        
        }
        //Filters JSON data into urls, which are pushed into result vectors
        for(auto &array : j_filtered["data"]) {
            //Removes double quotations from urls
            std::string cleanString = array["url"];
            cleanString.erase(remove( cleanString.begin(), cleanString.end(), '\"' ),cleanString.end());
            
            if (array["rating"] == "g") {
                gResults.push_back(cleanString);
            }
            else if (array["rating"] == "pg") {
                pgResults.push_back(cleanString);
            }
            else if (array["rating"] == "pg13") {
                 pg13Results.push_back(cleanString);
            }
            else {
                rResults.push_back(cleanString);
            }
            std::cout << cleanString << std::endl;
            currOffset++;
        }
        
    }
    std::vector<std::string> getGStickers() {
        return gResults;
    }
    std::vector<std::string> getPGStickers() {
        return pgResults;
    }
    std::vector<std::string> getPG13Stickers() {
        return pg13Results;
    }
    std::vector<std::string> getRStickers() {
        return rResults;
    }
    void clear() {
        criteria = "";
        count = 0;
        currOffset = 0 ;
        gResults.clear();
        pgResults.clear();
        pg13Results.clear();
        rResults.clear();
    }
    
    void setCriteria(std::string crit) {
        std::replace(crit.begin(), crit.end(), ' ', '+');
        criteria = crit;
    }
private:
    std::string criteria;
    int count;
    int currOffset;
    std::vector<std::string> gResults;
    std::vector<std::string> pgResults;
    std::vector<std::string> pg13Results;
    std::vector<std::string> rResults;
    

};


int main(int argc, const char * argv[]) {
    std::ios_base::sync_with_stdio(false);
    welcomeMessage();
    std::string cmd;
    std::cin >> cmd;
    StickerSearch stickers;
    do {
        if (cmd == "search") {
            std::string crit;
            std::ws(std::cin);
            getline(std::cin,crit);
            stickers.clear();
            stickers.setCriteria(crit);
            stickers.populateResults(stickers.getData());
        }
        else if (cmd == "next"){
            stickers.populateResults(stickers.getData());
        }
        else if (cmd == "cancel") {
            stickers.clear();
            std::cout << "Search cancelled." << std::endl;
        }
        else if (cmd == "g") {
            char yesNo;
            std::cout << "You have found " << std::to_string(stickers.getGStickers().size()) << " g stickers. Would you like to see the urls? (Y/N)" << std::endl;
            std::cin >> yesNo;
            if (yesNo == 'y' || yesNo == 'Y'){
                for(auto &url : stickers.getGStickers()) {
                    std::cout << url << std::endl;
                }
            }
        }
        else if (cmd == "pg") {
            char yesNo;
            std::cout << "You have found " << std::to_string(stickers.getPGStickers().size()) << " pg stickers. Would you like to see the urls? (Y/N)" << std::endl;
            std::cin >> yesNo;
            if (yesNo == 'y' || yesNo == 'Y'){
                for(auto &url : stickers.getPGStickers()) {
                    std::cout << url << std::endl;
                }
            }
            
        }
        else if (cmd == "pg13") {
            char yesNo;
            std::cout << "You have found " << std::to_string(stickers.getPG13Stickers().size()) << " pg13 stickers. Would you like to see the urls? (Y/N)" << std::endl;
            std::cin >> yesNo;
            if (yesNo == 'y' || yesNo == 'Y'){
                for(auto &url : stickers.getPG13Stickers()) {
                    std::cout << url << std::endl;
                }
            }
        }
        else if (cmd == "r") {
            char yesNo;
            std::cout << "You have found " << std::to_string(stickers.getRStickers().size()) << " r stickers. Would you like to see the urls? (Y/N)" << std::endl;
            std::cin >> yesNo;
            if (yesNo == 'y' || yesNo == 'Y'){
                for(auto &url : stickers.getRStickers()) {
                    std::cout << url << std::endl;
                }
            }
        }
        else if (cmd == "help") {
            helpMessage();
        }
        else {
            std::cout << "Invalid command." << std::endl;
        }
        std::cin >> cmd;
    }while (cmd != "end");
    
    
}
