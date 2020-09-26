#pragma once
#include<vector>
#include<iostream>

namespace ext {
    class Maze {
        protected:
            double EAST_WALL_THRESHOLD;
            double SOUTH_WALL_THRESHOLD;
            std::vector< std::tuple<int, int, int, int, int, int> > passages;
            typedef std::tuple<int, int, int> pos;

            int get(int, int, int);
            void set(int, int, int, int);
            void calculate();
        public:
            int LENGTH;
            int WIDTH;
            int HEIGHT;
            size_t seed;

            static const uint8_t FLOOR     =  1;   // 00 00 00 01;
            static const uint8_t EAST      =  2;   // 00 00 00 10;
            static const uint8_t NORTH     =  4;   // 00 00 01 00;
            static const uint8_t WEST      =  8;   // 00 00 10 00;
            static const uint8_t SOUTH     = 16;   // 00 01 00 00;
            static const uint8_t CEIL      = 32;   // 00 10 00 00;

            std::vector<int> cells;

            int& operator()(int row, int col, int floor){
                return cells.at(col + LENGTH*row + LENGTH*WIDTH*floor);
            }
            void initialize(int, int, int, double, double);
            void build();
            void print();
    };
}