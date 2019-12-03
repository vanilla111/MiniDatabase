SRC := $(shell ls src/*/*.cpp src/*.cpp)
OBJ := $(shell echo $(SRC) | sed 's/cpp/o/g')
FLAGS := -Wall -std=c++11 -I src -O2

main: $(OBJ)
	g++ $(OBJ) $(FLAGS) -o minisql
# 	@-mkdir data
# 	@-mkdir data/catalog
# 	@-mkdir data/index
# 	@-mkdir data/record
# 	rm -f $(OBJ)


%.o:%.cpp
	g++ -c $^ $(FLAGS) -o $@
