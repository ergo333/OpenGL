
module example001()
{
	
	box = [5,1,5];
	union(){
		cylinder(r=1, h=10, center=true); 
		translate([2, 0, 3]) {
			cube(box, true);
		}
	}
	
}

example001();

