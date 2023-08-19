//-------------------------------------------------------------------
// title: LaskaKit ESPink-42 ESP32 e-Paper BOX
// author: OK1HRA 
// license: Creative Commons BY-SA
// revision: 0.1
// format: OpenSCAD
//-------------------------------------------------------------------
// HowTo:
// After open change inputs parameters and press F5
// .STL export press F6 and menu /Design/Export as STL...
//-------------------------------------------------------------------
// PCB 91,44 x 87,62 mm
// hole 4 x 2,5 mm (86,36 x 82,55 mm)
//   translate([0, -87.62/2, 1.3]) import("e-ink-ESPink-42_v2.2.stl", convexity = 5);

//difference(){
front();
rotate([180,0,0]) translate([XX*2+10,0,-ZZ+2])
    back();
//    translate([-50,0,-5]) cube([150,50,30]);
//}


XX=86.36/2;
YY=82.55/2;
ZZ=9+3;
SPACE=0.35;


module back(){
    difference(){
        union(){
            difference(){
                union(){
                    hull(){
                        translate([XX,YY, 3])  cylinder(h=3+6, d=5+2*SPACE-0.2, center=false, $fn=60);
                        translate([-XX,YY, 3])  cylinder(h=3+6, d=5+2*SPACE-0.2, center=false, $fn=60);
                        translate([XX,-YY, 3])  cylinder(h=3+6, d=5+2*SPACE-0.2, center=false, $fn=60);
                        translate([-XX,-YY, 3])  cylinder(h=3+6, d=5+2*SPACE-0.2, center=false, $fn=60);
                    }
                }
                    hull(){
                        translate([XX,YY, 2])  cylinder(h=1+6, d=5+2*SPACE-0.2-2, center=false, $fn=60);
                        translate([-XX,YY, 2])  cylinder(h=1+6, d=5+2*SPACE-0.2-2, center=false, $fn=60);
                        translate([XX,-YY, 2])  cylinder(h=1+6, d=5+2*SPACE-0.2-2, center=false, $fn=60);
                        translate([-XX,-YY, 2])  cylinder(h=1+6, d=5+2*SPACE-0.2-2, center=false, $fn=60);
                    }
                    //- cut wall
                    hull(){
                        translate([0, YY*0.7,3])  rotate([0,90,0]) cylinder(h=XX*2.5, d=8, center=true, $fn=60);
                        translate([0, -YY*0.75,3])  rotate([0,90,0]) cylinder(h=XX*2.5, d=8, center=true, $fn=60);
                    }
                    hull(){
                        translate([XX*0.78, 0,3])  rotate([90,0,0]) cylinder(h=XX*2.5, d=8, center=true, $fn=60);
                        translate([-XX*0.7,0, 3])  rotate([90,0,0]) cylinder(h=XX*2.5, d=8, center=true, $fn=60);
                    }
                }
                    translate([-XX*0.5, 9.25, ZZ-3])  rotate([90,0,0])   cylinder(h=3.5, d=6, center=true, $fn=60);
                    translate([-XX*0.5, -9.25, ZZ-3])  rotate([90,0,0])   cylinder(h=3.5, d=6, center=true, $fn=60);
            }

        //-screw hole
        translate([XX-16,0, ZZ-3-1])  cylinder(h=3+2, d=7, center=false, $fn=30);
        hull(){
            translate([XX-16,0, ZZ-3-1])  cylinder(h=3+1.1, d2=3, d1=8, center=false, $fn=30);
            translate([XX-10,0, ZZ-3-1])  cylinder(h=3+1.1, d2=3, d1=8, center=false, $fn=30);
        }
        // -flip
        hull(){
            translate([-XX*0.5, 0, ZZ-3])  rotate([90,0,0])   cylinder(h=15.2, d=6.2, center=true, $fn=60);
            translate([-XX*0.5+2.7, 0, ZZ])  rotate([90,0,0])   cylinder(h=15.2, d=6.2, center=true, $fn=60);
        }
        hull(){
            translate([-XX*0.5, 0, ZZ-3])  rotate([90,0,0])   cylinder(h=15.2, d=6.2, center=true, $fn=60);
            translate([-XX*0.5, 0, ZZ-3])  rotate([90,0,0])   cylinder(h=15+6.2, d=1.2, center=true, $fn=60);
        }
        hull(){
            translate([-XX*0.5, 0, ZZ-1.5])  rotate([90,0,0])   cylinder(h=15.2, d=3.2, center=true, $fn=30);
            translate([-XX/4, 0, ZZ-1.5])   cylinder(h=3.1, d1=15.2, d2=19.4, center=true, $fn=60);
//            translate([-XX/4, 0, ZZ-1.5])  rotate([0,0,0]) cube([0.1, 15, 3.1], center=true);
        }
        hull(){
            translate([-XX/4, 0, ZZ-1.5])   cylinder(h=3.1, d1=15.2, d2=19.4, center=true, $fn=60);
            translate([XX/2, 20, ZZ-1.5])   cylinder(h=3.1, d1=15.2, d2=19.4, center=true, $fn=60);
        }
        hull(){
            translate([-XX/4, 0, ZZ-1.5])   cylinder(h=3.1, d1=15.2, d2=19.4, center=true, $fn=60);
            translate([XX/6, 0, ZZ-1.5])   cylinder(h=3.1, d1=10.2, d2=19.4, center=true, $fn=60);
        }
        hull(){
            translate([-XX/4, 0, ZZ-1.5])   cylinder(h=3.1, d1=15.2, d2=19.4, center=true, $fn=60);
            translate([XX/2, -20, ZZ-1.5])   cylinder(h=3.1, d1=15.2, d2=19.4, center=true, $fn=60);
        } 
        // -mark key
        translate([-XX+5,-YY+12, ZZ-0.5])  rotate([0,0,60]) cylinder(h=3+6, d=10, center=false, $fn=3);
        translate([-XX+16,-YY+12,ZZ-0.5]) rotate([0,0,90]) {
            linear_extrude(height = 3, center = false, convexity = 5, twist = -0, slices = 20, scale = 1.0) {
               text("USB", font = "Sans Uralic:style=Bold", halign="center", size=6);
            }
        }

    }
    // clip
    translate([-XX-2.5-SPACE+0.1, 0, ZZ-1.5])  rotate([0,45,0]) cube([2, 3, 2], center=true);
    translate([XX+2.5+SPACE-0.1, 0, ZZ-1.5])  rotate([0,45,0]) cube([2, 3, 2], center=true);
    translate([0,-YY-2.5-SPACE+0.1, ZZ-1.5])  rotate([45,0,0]) cube([3, 2, 2], center=true);
    translate([0,YY+2.5+SPACE-0.1, ZZ-1.5])  rotate([45,0,0]) cube([3, 2, 2], center=true);

    // flip
    union(){
        hull(){
            translate([-XX*0.5, 0, ZZ-3])  rotate([90,0,0])   cylinder(h=15, d=6, center=true, $fn=60);
            translate([-XX*0.5, 0, ZZ-3])  rotate([90,0,0])   cylinder(h=15+6, d=1, center=true, $fn=60);
        }
        hull(){
            translate([-XX*0.5, 0, ZZ-3])  rotate([90,0,0])   cylinder(h=15, d=6, center=true, $fn=60);
            translate([-XX/4, 0, ZZ-1.5])   cylinder(h=3, d1=15, d2=15, center=true, $fn=60);
        }
        hull(){
            translate([-XX*0.5, 0, ZZ-1.5])  rotate([90,0,0])   cylinder(h=15, d=3, center=true, $fn=30);
          translate([-XX/4, 0, ZZ-1.5])   cylinder(h=3, d1=15, d2=19, center=true, $fn=60);
        }
        hull(){
            translate([-XX/4, 0, ZZ-1.5])   cylinder(h=3, d1=15, d2=19, center=true, $fn=60);
            translate([XX/2, 20, ZZ-1.5])   cylinder(h=3, d1=15, d2=19, center=true, $fn=60);
        }
        hull(){
            translate([-XX/4, 0, ZZ-1.5])   cylinder(h=3, d1=15, d2=19, center=true, $fn=60);
            translate([XX/2, -20, ZZ-1.5])   cylinder(h=3, d1=15, d2=19, center=true, $fn=60);
        }
    }
}

module front(){
    difference(){
        union(){
            hull(){
                translate([XX,YY, 0])  cylinder(h=ZZ, d=5+2*SPACE+4, center=false, $fn=60);
                translate([-XX,YY, 0])  cylinder(h=ZZ, d=5+2*SPACE+4, center=false, $fn=60);
                translate([XX,-YY, 0])  cylinder(h=ZZ, d=5+2*SPACE+4, center=false, $fn=60);
                translate([-XX,-YY, 0])  cylinder(h=ZZ, d=5+2*SPACE+4, center=false, $fn=60);
            }
            hull(){
                translate([XX,YY, -2])  cylinder(h=2.01, d1=5+2*SPACE, d2=5+2*SPACE+4, center=false, $fn=60);
                translate([-XX,YY, -2])  cylinder(h=2.01, d1=5+2*SPACE, d2=5+2*SPACE+4, center=false, $fn=60);
                translate([XX,-YY, -2])  cylinder(h=2.01, d1=5+2*SPACE, d2=5+2*SPACE+4, center=false, $fn=60);
                translate([-XX,-YY, -2])  cylinder(h=2.01, d1=5+2*SPACE, d2=5+2*SPACE+4, center=false, $fn=60);
            }
        }
        hull(){
            translate([XX,YY, 0])  cylinder(h=ZZ+1, d=5+2*SPACE, center=false, $fn=60);
            translate([-XX,YY, 0])  cylinder(h=ZZ+1, d=5+2*SPACE, center=false, $fn=60);
            translate([XX,-YY, 0])  cylinder(h=ZZ+1, d=5+2*SPACE, center=false, $fn=60);
            translate([-XX,-YY, 0])  cylinder(h=ZZ+1, d=5+2*SPACE, center=false, $fn=60);
        }
        // - window
        //    hull(){
        //        translate([0,3.9,0]) cube([85,64,0.1], center=true);
        //        translate([0,3.9,-2]) cube([85+4,64+4,0.1], center=true);
        //    }
        RADIUS = 2; 
        LCDx=84.5-0.4;
        LCDy=62.5-0.4;
        Yshift=3.375;
        //
        hull(){
            translate([LCDx/2-RADIUS, LCDy/2-RADIUS+Yshift, -2.1])  cylinder(h=2.2, d1=RADIUS*2+4, d2=RADIUS*2, center=false, $fn=60);
            translate([-LCDx/2+RADIUS, LCDy/2-RADIUS+Yshift, -2.1])  cylinder(h=2.2, d1=RADIUS*2+4, d2=RADIUS*2, center=false, $fn=60);

            translate([LCDx/2-RADIUS, -LCDy/2+RADIUS+Yshift, -2.1])  cylinder(h=2.2, d1=RADIUS*2+4, d2=RADIUS*2, center=false, $fn=60);
            translate([-LCDx/2+RADIUS, -LCDy/2+RADIUS+Yshift, -2.1])  cylinder(h=2.2, d1=RADIUS*2+4, d2=RADIUS*2, center=false, $fn=60);
        }
        // - usb-c
        translate([-XX-2.5, -29.25, 3+1.6])  cube([5, 9.5, 3.7], center=true);
        // - microSD
        translate([-XX-2.5, -13.20, 3+1.45])  cube([5, 12, 1.5], center=true);
        translate([-XX-2.5-20.7, -13.20, 3+1.45])  scale([1,1.3,0.5]) sphere(r=20, $fn=200);
        // - pwr
        translate([-XX-2.5, 0, 3+0.6])  cube([5, 5, 2], center=true);
        // - res
        translate([-XX-2.5, 12.4, 3+0.85])  cube([5, 3, 2], center=true);
        // - back clip
        translate([-XX-2.5-SPACE, 0, ZZ-1.5])  rotate([0,45,0]) cube([2, 4, 2], center=true);
        translate([XX+2.5+SPACE, 0, ZZ-1.5])  rotate([0,45,0]) cube([2, 4, 2], center=true);
        translate([0,-YY-2.5-SPACE, ZZ-1.5])  rotate([45,0,0]) cube([4, 2, 2], center=true);
        translate([0,YY+2.5+SPACE, ZZ-1.5])  rotate([45,0,0]) cube([4, 2, 2], center=true);
    }

    // hole support
    difference(){
        union(){
            translate([XX,YY, 0])  cylinder(h=1.3, d=5+2*SPACE, center=false, $fn=60);
            translate([-XX,YY, 0])  cylinder(h=1.3, d=5+2*SPACE, center=false, $fn=60);
            translate([XX,-YY, 0])  cylinder(h=1.3, d=5+2*SPACE, center=false, $fn=60);
            translate([-XX,-YY, 0])  cylinder(h=1.3, d=5+2*SPACE, center=false, $fn=60);
        }
        hull(){
            translate([XX+2.5,YY, 0])  cylinder(h=1.51, d=1, center=false, $fn=60);
            translate([-XX-2.5,YY, 0])  cylinder(h=1.51, d=1, center=false, $fn=60);
            translate([XX+2.5,-YY, 0])  cylinder(h=1.51, d=1, center=false, $fn=60);
            translate([-XX-2.5,-YY, 0])  cylinder(h=1.51, d=1, center=false, $fn=60);
        }
    }

    // PCB clip
    hull(){
        translate([XX+2.5, 0, 3.1])  cube([1, 3, 0.1], center=true);
        translate([XX+2.5+0.5, 0, 3.1+4])  cube([0.1, 3, 0.1], center=true);
    }
    hull(){
        translate([-XX-2.5, 6.5, 3.1])  cube([1, 3, 0.1], center=true);
        translate([-XX-2.5-0.5, 6.5, 3.1+4])  cube([0.1, 3, 0.1], center=true);
    }
    hull(){
        translate([0, YY+2.5, 3.1])  cube([3, 1, 0.1], center=true);
        translate([0, YY+2.5+0.5, 3.1+4])  cube([3, 0.1, 0.1], center=true);
    }
        translate([-19, -YY-1.75, 5.5/2])  cube([3, 3, 5.5], center=true);
    hull(){
        translate([-19, -YY-0.25, 3.1])  cube([3, 1, 0.1], center=true);
        translate([-19, -YY-0.25, 3.1+2.3])  cube([3, 0.1, 0.1], center=true);
    }
}
