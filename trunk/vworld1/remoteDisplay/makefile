CLASSFILES = AvatarStatus.class AvatarGraphic.class ShowWorld.class 

%.class: %.java
	javac -cp ~/bin/java $<
	mv $@ ~/bin/java/remoteDisplay
 

all: ${CLASSFILES}

clean :
	rm -f *.class
