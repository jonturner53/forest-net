CLASSFILES = MazeAvatarStatus.class MazeAvatarGraphic.class AvatarStatus.class AvatarGraphic.class MazeWorld.class MazeWorld2.class

%.class: %.java
	javac -cp ~/bin/java $<
	mv $@ ~/bin/java/remoteDisplay
 

all: topoGen ${CLASSFILES}

topoGen:
	${MAKE} -C TopologyGen all

clean :
	${MAKE} -C TopologyGen clean	
	rm -f *.class
