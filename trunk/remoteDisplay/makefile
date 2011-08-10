CLASSFILES = MazeAvatarStatus.class MazeAvatarGraphic.class AvatarStatus.class AvatarGraphic.class MazeWorld.class MazeWorld2.class

%.class: %.java
	echo .. : ~/bin/java
	javac -cp ..:~/bin/java $<
	cp $@ ~/bin/java/remoteDisplay
 

all: topoGen ${CLASSFILES}

topoGen:
	${MAKE} -C TopologyGen all

clean :
	${MAKE} -C TopologyGen clean	
	rm -f *.class
