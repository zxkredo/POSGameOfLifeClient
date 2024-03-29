//
// Created by Tatry on 8. 1. 2024.
//

#include <sstream>
#include "../headers/ServerSaver.h"
#include "../headers/my_socket.h"

void ServerSaver::saveWorldToServer(const world_t& worldToSave, const std::string& fileName) {
    std::stringstream dataToSend;
    dataToSend << "w" << fileName << ServerSaver::endRowChar;
    for (const auto &row : worldToSave) {
        for (const bool &cell: row)
        {
            if (cell)
            {
                dataToSend << ServerSaver::trueCellChar;
            }
            else
            {
                dataToSend << ServerSaver::falseCellChar;
            }
        }
        dataToSend << ServerSaver::endRowChar;
    }
    MySocket* socket = MySocket::createConnection(HOSTNAME, PORT);
    socket->sendData(dataToSend.str());
    socket->sendEndMessage();
    delete socket;
}

bool ServerSaver::tryGetWorldFromServer(world_t& loadedWorld, const std::string& fileName) {
    MySocket* socket = MySocket::createConnection(HOSTNAME, PORT);
    std::stringstream dataToSend;
    dataToSend << "r" << fileName << ServerSaver::endRowChar;
    socket->sendData(dataToSend.str());

    std::string receivedData = socket->readData();
    socket->sendEndMessage();
    delete socket;
    return ServerSaver::parseWorldFromServer(receivedData, loadedWorld);
}

bool ServerSaver::parseWorldFromServer(std::string& receivedData, ServerSaver::world_t& world) {
    if (receivedData == "\n")
    {
        return false;
    }

    world = world_t {};
    world.emplace_back();
    std::vector<bool>* row = &world.back();
    const char* lastCharacter = &receivedData.c_str()[receivedData.size()-1];
    for (const char& character : receivedData)
    {
        if (character == ServerSaver::trueCellChar)
        {
            row->push_back(true);
        }
        else if (character == ServerSaver::falseCellChar)
        {
            row->push_back(false);
        }
        else if (character == ServerSaver::endRowChar)
        {
            if (&character != lastCharacter)
            {
                world.emplace_back();
                row = &world.back();
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}
