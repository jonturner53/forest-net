
CLASSFILES = MazeAvatarStatus.class MazeAvatarGraphic.class AvatarStatus.class AvatarGraphic.class MazeWorld.class MazeWorld2.class

%.class: %.java
	javac $<
	cp $@ ~/bin/java/remoteDisplay

all : ${CLASSFILES}

clean :
	rm -f *.class
