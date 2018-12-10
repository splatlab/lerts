import java.io.FileInputStream;
import java.io.FileOutputStream;
import com.yahoo.memory.Memory;
import com.yahoo.sketches.ArrayOfStringsSerDe;
import com.yahoo.sketches.frequencies.ErrorType;
import com.yahoo.sketches.frequencies.ItemsSketch;



public class TestFreq {

  public static void main(String[] args) {
    ItemsSketch<String> sketch1 = new ItemsSketch<String>(64);
    sketch1.update("a");
    sketch1.update("a");
    sketch1.update("b");
    sketch1.update("c");
    sketch1.update("a");
    sketch1.update("d");
    sketch1.update("a");

    System.out.println("Estimate a: " + sketch1.getEstimate("a"));
    System.out.println("Estimate b: " + sketch1.getEstimate("b"));
    System.out.println("Estimate c: " + sketch1.getEstimate("c"));
    System.out.println("Estimate d: " + sketch1.getEstimate("d"));
  }
}
