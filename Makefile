CXX=g++
INCS=-I.
CXXFLAGS = -std=c++11 -g $(INCS)
RM=rm -f

CLIENTFILES = whatsappClient.cpp whatsappio.cpp
SERVERFILES = whatsappServer.cpp whatsappio.cpp
SRCFILES = whatsappServer.cpp whatsappClient.cpp whatsappio.cpp whatsappio.h
ADDITIONALFILES = README Makefile

TAR=tar
TARFLAGS=-cvf
TARNAME=ex4.tar
TARSRCS=$(SRCFILES) $(ADDITIONALFILES)
TARGETS = whatsappClient whatsappServer


all: makeClient makeServer

makeClient: $(CLIENTFILES)
	$(CXX) $(CXXFLAGS) $(CLIENTFILES) -o whatsappClient

makeServer: $(SERVERFILES)
	$(CXX) $(CXXFLAGS) $(SERVERFILES) -o whatsappServer

clean:
	$(RM) $(TARGETS) *~ *core ex4.tar

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
