
CLASSFILES = Pair.class Link.class Interface.class Comtree.class Controller.class NetObj.class Panel.class Router.class ComtreeDisplay.class MazeAvatarStatus.class MazeAvatarGraphic.class AvatarStatus.class AvatarGraphic.class MazeWorld.class MazeWorld2.class

%.class: %.java
	javac $<
	cp $@ ~/bin/java/remoteDisplay

all : ${CLASSFILES}

clean :
	rm -f *.class
