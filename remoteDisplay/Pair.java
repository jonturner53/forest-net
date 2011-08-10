/** @file Pair.java */

package remoteDisplay;

/**
* Custom Python style Pair class useful for storing a pair of values as a key in a Map. In ComtreeDisplay, this is used as a key to map a forest address and a comtree number. 
*/
public class Pair implements Comparable{
	private String first, last, cat; ///< first and last are the two values stored in this Pair structure. cat is the appending of both first and last.	
	public Pair(String f, String l){
		first = f;
		last = l;
		cat = f+l;
	}	
	
	/**
	* @return first part of the Pair
	*/
	public String getFirst(){
		return first;
	}
	
	/**
	* @return last part of the Pair
	*/
	public String getLast(){
		return last;
	}
	
	@Override
	public boolean equals(Object obj){
		return this.compareTo(obj)==0;
	}

	@Override
	public int compareTo(Object obj){
		Pair p = ((Pair) obj);
		
		/*
		int cmp1 = first.compareTo(p.getFirst());
		int cmp2 = last.compareTo(p.getLast());
		if (cmp1 == 0 && cmp2 == 0) return 0;
		else if (cmp1 == -1 || cmp1 == 0 && cmp2 == -1) return -1;
		else return 1;
		*/
		return cat.compareTo(p.cat);
	}

	@Override
	public String toString(){
		return new String("Pair: " + first + " " + last);
	}
}
