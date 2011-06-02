
CLASSFILES = ShowWorld.class AvatarGraphic.class AvatarStatus.class

%.class: %.java
	javac $<
	cp $@ ~/bin/java/remoteDisplay

all : ${CLASSFILES}

AvatarGraphic.class : AvatarStatus.class

ShowWorld.class : AvatarGraphic.class

clean :
	rm -f *.class
