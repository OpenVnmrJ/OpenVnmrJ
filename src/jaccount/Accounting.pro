#
# This ProGuard configuration file illustrates how to process applications.
# Usage:
#     java -jar proguard.jar @applications.pro
#

# target app for Java 1.6, thus add the preverifing the code, app loads faster
-target 1.6

-verbose 

# "Print out a list of what we're preserving."
# -printseeds

# always do analysis, etc.
-forceprocessing

# -ignorewarnings

#  without this option the SConstrcut build vnmrj.jar input hangs at the preverify step
# -dontpreverify

-dontskipnonpubliclibraryclasses


# Specify the input jars, output jars, and library jars.

# read in the account.jar for analysis
-injars  account.jar
# output directory for new obfuscated account.jar file
-outjars account.prodir

-printmapping account.pro.map

# the following for useful stack-traces
-renamesourcefileattribute SourceFile
-keepattributes SourceFile,LineNumberTable, Exceptions, InnerClasses, Signature
# Preserve all annotations.
# -keepattributes *Annotation*

# #-libraryjars <java.home>/lib/rt.jar

-libraryjars ../../3rdParty/java/jre/lib/rt.jar

# Preserve all public applications.
-keepclasseswithmembers public class * {
    public static void main(java.lang.String[]);
}

# Preserve all native method names and the names of their classes.
-keepclasseswithmembernames class * {
    native <methods>;
}

# Preserve the special static methods that are required in all enumeration
# classes.
-keepclassmembers class * extends java.lang.Enum {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}

# Explicitly preserve all serialization members. The Serializable interface
# is only a marker interface, so it wouldn't save them.
# You can comment this out if your application doesn't use serialization.
# If your code contains serializable classes that have to be backward 
# compatible, please refer to the manual.
-keepclassmembers class * implements java.io.Serializable {
    static final long serialVersionUID;
    static final java.io.ObjectStreamField[] serialPersistentFields;
    private void writeObject(java.io.ObjectOutputStream);
    private void readObject(java.io.ObjectInputStream);
    java.lang.Object writeReplace();
    java.lang.Object readResolve();
}

# "Swing UI look and feels are implemented as extensions of the ComponentUI class. "
# "For some reason, these have to contain a static method createUI, which the Swing API "
# "invokes using introspection. You should therefore always preserve the method "
# "as an entry point, for instance like this: "
-keep class * extends javax.swing.plaf.ComponentUI {
    public static javax.swing.plaf.ComponentUI createUI(javax.swing.JComponent);
}

# Your application may contain more items that need to be preserved; 
# typically classes that are dynamically created using Class.forName:
# -keep public class mypackage.MyClass
# -keep public interface mypackage.MyInterface
# -keep public class * implements mypackage.MyInterface

# resource files, it may be necessary to adapt their names and/or 
# their contents when the application is obfuscated. The following two 
# options can achieve this automatically: 

# -adaptresourcefilenames    **.properties,**.gif,**.jpg
# -adaptresourcefilecontents **.properties,META-INF/MANIFEST.MF

