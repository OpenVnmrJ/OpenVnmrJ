/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.jgl;

public class FColor {
	public static double min_brightness=0.5;
	public static double min_contrast=0.4;

	public float  rgba[]=new float[4];
	FColor(double r,double g,double b){
		rgba[0]=(float)r;
		rgba[1]=(float)g;
		rgba[2]=(float)b;
		rgba[3]=1.0f;
	}
	FColor(double r,double g,double b, double a){
		rgba[0]=(float)r;
		rgba[1]=(float)g;
		rgba[2]=(float)b;
		rgba[3]=(float)b;
	}
	FColor(FColor c){
		rgba[0]=c.rgba[0];
		rgba[1]=c.rgba[1];
		rgba[2]=c.rgba[2];
		rgba[3]=c.rgba[3];
	}
	FColor(float c[]){
		rgba[0]=c[0];
		rgba[1]=c[1];
		rgba[2]=c[2];
		rgba[3]=c[3];
	}
	double red()					{	return rgba[0];}
	double green()					{	return rgba[1];}
	double blue()					{	return rgba[2];}
	double alpha()					{	return rgba[3];}
	void set_red(double c)			{	rgba[0]=(float)c;}
	void set_green(double c)		{	rgba[1]=(float)c;}
	void set_blue(double c)			{	rgba[2]=(float)c;}
	void set_alpha(double c)		{	rgba[3]=(float)c;}
	
	FColor complement()				{	return new FColor(1.0f-red(),1.0f-green(),1.0f-blue());}
	double difference(FColor c){
		double db=Math.abs(brightness()-c.brightness())/min_brightness;
		double dc=Math.abs(contrast(c))/min_contrast;
		return max(db,dc);		
	}

	FColor mix(FColor c,double t){
		FColor c1=new FColor(c);
		FColor c2=new FColor(red(),green(),blue());
		c1=c1.mul(t);
		c2=c2.mul(1.0-t);
		FColor c3=c1.add(c2);
		return new FColor(c3.red(),c3.green(),c3.blue(),c3.alpha());
	}

	FColor add(FColor c)	    {   return new FColor(red()+c.red(),green()+c.green(),blue()+c.blue());}
	
	FColor mul(double s)	    {   return new FColor(red()*s,green()*s,blue()*s);}
	double brightness(){
		return ((red()*0.299) + (green()*0.587) + (blue()*0.114));
	}

	FColor clamp(){
		double r=max(red(),0.0);
		r=min(r,1.0);
		double g=max(green(),0.0);
		g=min(g,1.0);
		double b=max(blue(),0.0);
		b=min(b,1.0);
		double a=max(alpha(),0.0);
		a=min(a,1.0);
		return new FColor(r,g,b,a);
	}

	double contrast(FColor c){
		double diff=max(red(), c.red()) - min(red(), c.red())
		+ max(green(), c.green()) - min(green(), c.green())
		+ max(blue(), c.blue()) - min(blue(), c.blue());
		return diff/3.0;
	}
	static public FColor BLACK = new FColor(0.0,0.0,0.0);
	static public FColor WHITE = new FColor(1.0,1.0,1.0);
	
	FColor contrast_color(FColor fg,double ampl){
		FColor bg=new FColor(red(),green(),blue());
		double diff=fg.difference(bg);
		if(diff<1.0){
			FColor fg1=bg.mix(fg,ampl).clamp();
			double diff2=fg1.difference(bg);
			if(diff2>1)
				fg=fg1;
			else if(diff2<diff)
				fg=fg.complement();
			else
				fg=fg1.complement();
		}
		return new FColor(fg.red(),fg.blue(),fg.green());
	}


	double max(double a,double b) { return a>b?a:b;}
	double min(double a,double b) { return a<b?a:b;}
}
