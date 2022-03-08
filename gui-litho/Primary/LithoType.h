#pragma once
#include <iostream>
#include <string>

namespace litho
{
	struct LithoSetting
	{
		int	  strip_width = 1000;//default 1000
		int	  block_height = 1000;

		std::string stl_path;
		std::string xml_path;
		bool  along_x = true;

		// external
		float pixel_size_external=100;			// nanometer 
		float size_external;					// micrometer
		float thickness_external;				// micrometer
		float shell_thickness_external = 1;     // pixels
		float infill_grid_spacing_external = 3; // pixels
		float infill_rate = 0.5;			    // 100% infilling by default
		

		// internal
		float micrometer_internal = 1.f; // means 1mm in real world equals to X in data value.
		float pixel_size_internal;
		float size_internal;
		float thickness_internal;
		float shell_thickness_internal;
		float infill_grid_spacing_internal;


		void GetInternal()
		{
			float thickness_best_for_slic3r = 0.5;
			micrometer_internal =  thickness_best_for_slic3r/thickness_external;

			pixel_size_internal = pixel_size_external * 1e-3 * micrometer_internal;
			size_internal = size_external * micrometer_internal;
			thickness_internal = thickness_external * micrometer_internal;
			shell_thickness_internal = shell_thickness_external * pixel_size_internal;
			infill_grid_spacing_internal = infill_grid_spacing_external * pixel_size_internal;

			std::cout
				<<"internal para: "
				<<std::endl<<" "
				<< " pixel-size: "<<pixel_size_internal    
				<< " size: "<<size_internal			 
				<< " thickness: "<<thickness_internal 
				<< " shell-thickness: "<< shell_thickness_internal
				<< " "<<std::endl;
		}
	};

}