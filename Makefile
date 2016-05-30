# automatic variables: $@ = rule target, $< = first prereq , $^ = all prereq
%.o : %.cpp
	g++ -g -c $< 

game: typinggame.o
	g++ -g $^ -o $@ -lncurses
