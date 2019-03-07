
import javax.swing.JOptionPane;

public class dialog {
    public static void main(String[] args) {

        if (args.length > 0)
           JOptionPane.showMessageDialog(null, args[0]);
        else
           JOptionPane.showMessageDialog(null, "");
    }

}
