import java.util.*;

import java.io.*;
import java.io.FileNotFoundException;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import com.yahoo.memory.Memory;
import com.yahoo.sketches.ArrayOfStringsSerDe;
import com.yahoo.sketches.frequencies.*;



public class TestFreq {

  public static void main(String[] args) {
    int size = (1 << Integer.parseInt(args[0]));
    String filename = args[1];

    LongsSketch sketch1 = new LongsSketch(size);

    BufferedReader reader = null;
    List<Long> itemList = new ArrayList<Long>();
    try {
      reader = new BufferedReader(new FileReader(filename));
      String text = null;
      while ((text = reader.readLine()) != null) {
        itemList.add(Long.parseLong(text));
      }
    } catch (FileNotFoundException e) {
      e.printStackTrace();
    } catch (IOException e) {
      e.printStackTrace();
    } finally {
      try {
        if (reader != null) {
          reader.close();
        }
      } catch (IOException e) {
      }
    }
    int nitems = itemList.size();
    System.out.println("Nitems: " + nitems);

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
