# Overview
StickerApplication.cpp uses the GIPHY API search endpoint to return the URLs of the first 10 stickers from GIPHY.com, given a user keyword. The application will continue to pull stickers from GIPHY until a new search criteria is given. The application can also filter out these results by their specified ranking (g, pg, pg-13, r).

#Libraries Used 
StickerApplication.cpp uses Asio (non-boost) to pull data from the GIPHY API search endpoint and the nlohmann JSON library to parse out the resulting data. 
