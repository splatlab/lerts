import java.util.*;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import com.yahoo.memory.Memory;
import com.yahoo.sketches.ArrayOfStringsSerDe;
import com.yahoo.sketches.frequencies.*;



public class TestFreq {

  public static void main(String[] args) {
    int Max = (1 << 30);
    int size = (1 << Integer.parseInt(args[0]));
    int nitems = (int)(10*size);

    LongsSketch sketch1 = new LongsSketch(size);

    List<Long> itemList = new ArrayList<Long>();
    System.out.println("Nitems: " + nitems + " Size: " + size);
    for (int i = 0; i < nitems; i++) {
      long item = (long)(Math.random() * Max);
      //System.out.println("Inserting " + item);
      itemList.add(item);
    }

    long startTime = System.currentTimeMillis();
    for (int i = 0; i < itemList.size(); i++) {
      sketch1.update(itemList.get(i));
      //sketch1.getEstimate(itemList.get(i));
    }
    long endTime = System.currentTimeMillis();
    double total_time = (endTime - startTime) / 1000.0;
    System.out.println("Total time: " + total_time + " seconds");
    System.out.println("Throughput " + nitems/total_time + " inserts/seconds");
  }
}
