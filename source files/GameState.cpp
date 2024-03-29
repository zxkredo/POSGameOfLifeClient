//
// Created by Tatry on 1. 1. 2024.
//

#include "../headers/GameState.h"

GameState::GameState() : stopThreads(false), isGenerating(false), alreadyCheckedCells(0), toBeCheckedRow(0),
    toBeCheckedColumn(0), world2(), world1(), rows(0), columns(0), cellCount(0)
{
    this->currentWorld = &this->world2;
    this->futureWorld = &this->world1;
}

void GameState::start_generating_future_world() {
    std::unique_lock<std::mutex> lock(this->mutexGenerating);
    while (this->isGenerating) {
        finishedGenerating.wait(lock);
    }
    {
        std::unique_lock lock1(this->mutexToBeChecked);
        this->toBeCheckedRow = 0;
        this->toBeCheckedColumn = 0;
        this->alreadyCheckedCells = 0;
    }
    this->isGenerating = true;
    this->startGenerating.notify_all();
}

std::vector<std::vector<bool>> GameState::retrieve_current_world() {
    std::unique_lock<std::mutex> lock(this->mutexGenerating);
    while (this->isGenerating) {
        finishedGenerating.wait(lock);
    }
    return *this->currentWorld;
}

void GameState::insert_current_world(const std::vector<std::vector<bool>> &starting_world) {
    std::unique_lock<std::mutex> lock(this->mutexGenerating);
    while (this->isGenerating) {
        finishedGenerating.wait(lock);
    }

    *this->currentWorld = starting_world;
    *this->futureWorld = starting_world;

    this->rows = starting_world.size();
    this->columns = this->rows == 0 ? 0 : this->currentWorld->at(0).size();
    this->cellCount = this->rows * this->columns;
}

void GameState::start_checking_cells() {
    while (true) {
        unsigned int rowToCheck = 0;
        unsigned int columnToCheck = 0;
        get_cell_coordinates_to_check(rowToCheck, columnToCheck);

        //to be able to join with main thread
        if (stopThreads)
        {
            break;
        }

        //calculate cell state
        unsigned int sumOfAlive = 0;

        unsigned int rowStart = (rowToCheck == 0 ? rowToCheck : rowToCheck - 1);
        unsigned int rowEnd = (rowToCheck + 1 < this->rows ? rowToCheck + 1: rowToCheck);
        unsigned int colStart = (columnToCheck == 0 ? columnToCheck : columnToCheck - 1);
        unsigned int colEnd = (columnToCheck + 1 < this->columns? columnToCheck + 1: columnToCheck);
        for (unsigned int row = rowStart; row <= rowEnd; ++row)
        {
            for (unsigned int col = colStart; col <= colEnd; ++col)
            {
                if (get_current_world_cell_value(row, col))
                    sumOfAlive++;
                if (sumOfAlive > 4)
                    break;
            }
        }

        //set cell state

        //	If the sum of all nine fields in a given neighbourhood is three,
        //	the inner field state for the next generation will be life;
        //	if the all-field sum is four, the inner field retains its current state;
        //	and every other sum sets the inner field to death.
        if (sumOfAlive == 3)
        {
            update_future_world_cell(rowToCheck, columnToCheck, true);
        }
        else if (sumOfAlive != 4)
        {
            update_future_world_cell(rowToCheck, columnToCheck, false);
        }
        else
        {
            update_future_world_cell(rowToCheck, columnToCheck,
                                     get_current_world_cell_value(rowToCheck, columnToCheck));
        }
    }
}

void GameState::get_cell_coordinates_to_check(unsigned int& row, unsigned int& column) {
    //if outside of world (that is only columns, because rows are always valid),
    // wait until the worlds are swapped back = tobeChecked values are reset
    std::unique_lock lock(this->mutexToBeChecked);
    while (this->toBeCheckedColumn >= this->columns) {
        this->startGenerating.wait(lock);
        //if awoken when thread needs to terminate, need to exit function
        if (this->stopThreads)
        {
            return;
        }
    }

    row = this->toBeCheckedRow;
    column = this->toBeCheckedColumn;

    //move by one row
    this->toBeCheckedRow++;

    // Set valid coordinates if outside of world for rows
    if (this->toBeCheckedRow >= this->rows)
    {
        this->toBeCheckedRow = 0;
        this->toBeCheckedColumn++;
    }
}

bool GameState::get_current_world_cell_value(const unsigned int& row, const unsigned int& column) {
    return this->currentWorld->at(row).at(column);
}

void GameState::update_future_world_cell(const unsigned int& row, const unsigned int& column, bool cellState) {
    std::unique_lock lock(this->mutexFutureWorld);
    this->futureWorld->at(row).at(column) = cellState;
    this->alreadyCheckedCells++;

    if (this->alreadyCheckedCells >= this->cellCount)
    {
        this->swap_worlds();
    }
}

void GameState::stop_simulation() {
    stopThreads = true;
    this->startGenerating.notify_all();
}

void GameState::swap_worlds() {

    std::swap(futureWorld, currentWorld);

    {
        std::unique_lock lock1(mutexGenerating);
        this->isGenerating = false;
    }

    this->finishedGenerating.notify_one();
}
