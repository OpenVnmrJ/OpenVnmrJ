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

-ignorewarnings

#  without this option the SConstrcut build vnmrj.jar input hangs at the preverify step
# -dontpreverify

# at present an exception is thrown (below) by the optimization stage, so skip it for now
# java.lang.UnsupportedOperationException: Method must be overridden in [proguard.optimize.peephole.ClassMerger] if ever called
#        at proguard.classfile.util.SimplifiedVisitor.visitAnyClass(SimplifiedVisitor.java:47)
#        at proguard.classfile.util.SimplifiedVisitor.visitLibraryClass(SimplifiedVisitor.java:59)
#        at proguard.classfile.LibraryClass.accept(LibraryClass.java:248)
#        at proguard.classfile.ProgramClass.subclassesAccept(ProgramClass.java:368)
#        at proguard.optimize.peephole.VerticalClassMerger.visitProgramClass(VerticalClassMerger.java:83)
#        at proguard.classfile.ProgramClass.accept(ProgramClass.java:281)
#        at proguard.classfile.ClassPool.classesAccept(ClassPool.java:114)
#        at proguard.optimize.Optimizer.execute(Optimizer.java:296)
#        at proguard.ProGuard.optimize(ProGuard.java:325)
#        at proguard.ProGuard.execute(ProGuard.java:114)
#        at proguard.ProGuard.main(ProGuard.java:499)
-dontoptimize

-dontskipnonpubliclibraryclasses


# Specify the input jars, output jars, and library jars.

# -injars  vnmrj.jar(!com/**,!org/**,!sunw/**,!javax/**,!vjmol/**,!jmf/**,!vjmol/VJMol/**)
# -injars  vnmrj.jar(!com/**,!org/**,!sunw/**,!vjmol/**,!jmf/**,!vjmol/VJMol/**)

# read in the managedb.jar for analysis
-injars  managedb.jar
# output directory for new obfuscated managedb.jar file
-outjars managedb.prodir

-printmapping managedb.pro.map

# the following for useful stack-traces
-renamesourcefileattribute SourceFile
-keepattributes SourceFile,LineNumberTable, Exceptions, InnerClasses, Signature
# Preserve all annotations.
# -keepattributes *Annotation*

# #-libraryjars <java.home>/lib/rt.jar
## "the below doesn't work, too many missing classes"
## -libraryjars ../../3rdParty/java/jre/lib/rt.jar(!com/**,!org/**,!sunw/**,!javax/**)
## -libraryjars ../../3rdParty/java/jre/lib/rt.jar(!com/sun/**,!com/sun/org/apache/**,!org/xml/**,!org/w3c/**,!sun/com/**,!javax/xml/**)
##-libraryjars ../../3rdParty/java/jre/lib/rt.jar( !com/sun/glugen/runtime,!com/sun/opengl/cp/**, !com/sun/opengl/imp/**,com/sun/opengl/util/**, !com/sun/xml/parser/**, !com/sun/xml/tree/**, !com/sun/xml/util/**, !com/sun/java/help/impl/**, !com/sun/java/help/search/**, !javax/help/**, !javax/media/opengl/**, !org/w3c/dom/**, !org/xml/sax/** )

# use ignorewarnings to allow processing to happen.
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

## keep all the com/ org/ sunw/ javax/ vjmol/ jmf/ vjmol.VJMol.* classes, 
## and do not mangle the method names
-keep class com.** 
-keepnames class com.** {
    public protected <fields>;
    public protected <methods>;
}
-keep class org.** 
-keepnames class org.** {
    public protected <fields>;
    public protected <methods>;
}
-keep class sunw.** 
-keepnames class sunw.** {
    public protected <fields>;
    public protected <methods>;
}
-keep class javax.**
-keepnames class javax.** {
    public protected <fields>;
    public protected <methods>;
}
-keep class vjmol.**
-keepnames class vjmol.** {
    public protected <fields>;
    public protected <methods>;
}
-keep class jmf.** 
-keepnames class jmf.**  {
    public protected <fields>;
    public protected <methods>;
}
-keep class vjmol.VJMol.**
-keepnames class vjmol.VJMol.** {
    public protected <fields>;
    public protected <methods>;
}
-keep class postgresql.**
-keepnames class postgresql.** {
    public protected <fields>;
    public protected <methods>;
}

# ManageDB classes and methods not to managle (if used)

# -keep
-keepnames public class vnmr.templates.Types {
    public <methods>;
}

# -keep
-keepnames public class vnmr.bo.User {
    public protected <fields>;
    public protected <methods>;
}

# -keep
-keepnames public class vnmr.bo.VMessage {
    public protected <fields>;
    public protected <methods>;
}
# -keep
-keepnames public class vnmr.templates.VElement {
    public protected <fields>;
    public protected <methods>;
}
# -keep
-keepnames public class vnmr.templates.VCommandDefinition {
    public protected <fields>;
    public protected <methods>;
}

## "ManageDB classes to keep and not mangle even if they don't appear to be used."

# resource files, it may be necessary to adapt their names and/or 
# their contents when the application is obfuscated. The following two 
# options can achieve this automatically: 

# -adaptresourcefilenames    **.properties,**.gif,**.jpg
# -adaptresourcefilecontents **.properties,META-INF/MANIFEST.MF

