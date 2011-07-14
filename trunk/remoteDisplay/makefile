
CLASSFILES = Pair.class ComtreeDisplay.class Comtree.class Controller.class Interface.class Link.class NetObj.class Router.class MazeWorld.class AvatarGraphic.class AvatarStatus.class MazeAvatarStatus.class MazeAvatarGraphic.class MazeWorld2.class

%.class: %.java
	echo $$CLASSPATH $$HOME
	javac $<
	cp $@ ~/bin/java/remoteDisplay

all : ${CLASSFILES}

clean :
	rm -f *.class
