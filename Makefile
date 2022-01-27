# automatic variables: $@ = rule target, $< = first prereq , $^ = all prereq
%.o : %.cpp
	g++ -g -c $< 

game: typinggame.o
	g++ -std=c++11 -g $^ -o $@ -lncurses
