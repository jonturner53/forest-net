
""" Places.py

Date: 2013
Authors: Jon Turner
"""

# Places in demoWorld with locations, neighbors

place = [[]]*100
place[1] = [(63,84), [2,4,19]]
place[2] = [(63,67), [1,3,13,14]]
place[3] = [(83,67), [2,4,10]]
place[4] = [(83,84), [1,3,5]]
place[5] = [(77,112), [4,6,18]]
place[6] = [(98,113), [5,7]]
place[7] = [(99,84), [6,8,9]]
place[8] = [(116,63), [7,9]]
place[9] = [(93,62), [7,8,10]]
place[10] = [(83,59), [3,9,11]]
place[11] = [(84,42), [10,12,24]]
place[12] = [(75,40), [11,13,24]]
place[13] = [(62,47), [2,12,23]]
place[14] = [(33,67), [2,15,20,23]]
place[15] = [(20,83), [14,16]]
place[16] = [(8,119), [15,17]]
place[17] = [(39,119), [16,18]]
place[18] = [(57,113), [5,17,19]]
place[19] = [(58,87), [1,18]]
place[20] = [(23,56), [14,21]]
place[21] = [(22,31), [20,22]]
place[22] = [(38,26), [21,23,24]]
place[23] = [(39,45), [13,14,22]]
place[24] = [(71,20), [11,12,22]]

""" Get the number of places that have been defined
"""
def getNumPlaces() : return 24 

""" Get the location associated with a given place.
"""
def getLoc(i) : return place[i][0]

""" Get the neighbors of a given place
"""
def getNeighbors(i) : return place[i][1]

