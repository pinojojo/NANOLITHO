#pragma once
#include <string>

namespace litho 
{
	struct LithoSetting
	{
		

		float scale_to_size;
		bool  along_x;

		float layer_num;
		float layer_thickness;
		bool  fixed_thickness;
		
		int	  strip_width;
		
		float pixel_size = 100.0e-6;// units: mm 

		bool check()
		{

		}
	};

}