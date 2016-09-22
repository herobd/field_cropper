#CXXFLAGS	= -O3 -fPIC -Wall
CXXFLAGS	= -g   -std=c++0x -fPIC -Wall

#On marylou5, use the INCPATH and LDPRJ below instead!
ifeq ($(USER),djk2)
INCPATH		= -I../../src -I/opt/intel/mkl/10.2.1.017/include/fftw 
LDPRJ           = -L../../lib -L/usr/lib -ldocumentproject -L/fslhome/djk2/libpng-1.2.40/installed/lib -ljpeg -ltiff -lmkl_intel_lp64 -lmkl_core -liomp5 -lmkl_gnu_thread -lpthread -lpng
BINPATH		= ../../bin
else
INCPATH		= -I../../src -I/usr/include/ 
LDPRJ		= -L../../lib -ldocumentproject -ljpeg -ltiff -lpng -lm -lfftw3 -lpthread 
BINPATH		= ../../bin
endif

#put all first if you want all to be made by default, or 'make all' will do it

all: field_cropper tester

IRIS_File.o: IRIS_File.h IRIS_File.cpp
	g++ `xml2-config --cflags --libs` -c $(CXXFLAGS) $(LDPRJ) $(INCPATH) IRIS_File.cpp

field_cropper: document_to_field_images.cpp IRIS_File.h IRIS_File.o
	g++ document_to_field_images.cpp -o $(BINPATH)/field_cropper IRIS_File.o `xml2-config --cflags --libs` $(CXXFLAGS) $(LDPRJ) $(INCPATH) 

tester: testIRIS.cpp IRIS_File.h IRIS_File.o
	g++ testIRIS.cpp -o $(BINPATH)/tester IRIS_File.o `xml2-config --cflags --libs` $(CXXFLAGS) $(LDPRJ) $(INCPATH) 

clean:
	rm $(BINPATH)/field_cropper 
	rm $(BINPATH)/tester
	rm IRIS_File.o
