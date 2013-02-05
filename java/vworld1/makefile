JAVAC=javac
CLASSFILES = AvatarStatus.class AvatarGraphic.class ShowWorld.class \
	DriveAvatar.class ShowWalls.class EditMap.class EMapPanel.class

%.class: %.java
	${JAVAC} -nowarn -cp ~/bin/java $<
	cp *.class ~/bin/java/forest/vworld1

all : ${CLASSFILES}

clean :
	rm -f *.class 
