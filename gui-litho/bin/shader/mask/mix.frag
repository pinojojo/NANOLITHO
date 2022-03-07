#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D contour_tex;
uniform sampler2D hole_tex;

void main()
{
    
    
    vec3 val_contour = texture(contour_tex, TexCoords).rgb;
    vec3 val_hole = texture(hole_tex, TexCoords).rgb;
    
    FragColor = vec4(0,0,0, 1.0);
    float final_val=0;

    

    if(val_contour.r>0)
    {
        if(val_hole.r>0)
        {   
            final_val=0;
        }
        else
        {
            final_val=val_contour;
        }
    }

    
    FragColor=vec4(final_val,final_val,final_val,1.0);
   
        
    

    
} 