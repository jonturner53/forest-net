CLASSFILES = NetObj.class Pair.class Link.class Interface.class Comtree.class Controller.class Router.class ComtreeDisplay.class MazeAvatarStatus.class MazeAvatarGraphic.class AvatarStatus.class AvatarGraphic.class MazeWorld.class MazeWorld2.class

%.class: %.java
	javac -cp .. $<
	cp $@ ~/bin/java/remoteDisplay
 

all: topoGen ${CLASSFILES}

topoGen:
	${MAKE} -C TopologyGen all

clean :
	${MAKE} -C TopologyGen clean	
	rm -f *.class
