
%.class: %.java
	javac $<

CLASSFILES = ShowWorld.class AvatarGraphic.class AvatarStatus.class

AvatarGraphic.class : AvatarStatus.class
ShowWorld.class : AvatarGraphic.class

all : ${CLASSFILES}
	cp ${CLASSFILES} ~/bin/java/remoteDisplay

clean :
	rm -f *.class
