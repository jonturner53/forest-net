
CLASSFILES = MazeWorld.class MazeAvatarGraphic.class MazeAvatarStatus.class

%.class: %.java
	echo $$CLASSPATH $$HOME
	javac $<
	cp $@ ~/bin/java/remoteDisplay

all : ${CLASSFILES}

AvatarGraphic.class : AvatarStatus.class

ShowWorld.class : AvatarGraphic.class

clean :
	rm -f *.class
