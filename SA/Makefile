online_cap: online_cap.o predict.o dump-util.o
	g++ online_cap.o predict.o dump-util.o -o online_cap
overhead: overhead.o predict.o dump-util.o
	g++ overhead.o predict.o dump-util.o -o overhead
get_access: get_access.o dump-util.o
	g++ get_access.o dump-util.o -o get_access
test_occu: test_occu.o predict.o dump-util.o
	g++ test_occu.o dump-util.o predict.o -o test_occu 
get_access.o: get_access.cpp simple-pebs.h dump-util.h
	g++ -c get_access.cpp
online_cap.o: online_cap.cpp simple-pebs.h dump-util.h predict.h
	g++ -c online_cap.cpp
test_occu.o: test_occu.cpp simple-pebs.h dump-util.h predict.h
	g++ -c test_occu.cpp
predict.o: predict.cpp predict.h
	g++ -c predict.cpp
dump-util.o: dump-util.cpp dump-util.h simple-pebs.h
	g++ -c dump-util.cpp
clean:
	rm -rf *.o online_cap get_access
