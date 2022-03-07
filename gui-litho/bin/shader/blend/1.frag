#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D infill_tex;
uniform sampler2D stroke_tex;
uniform sampler2D mask_tex;


void main()
{
     
    FragColor = vec4(0,0,0, 1.0);
    float final_val=0;
    
    vec3 val_infill = texture(infill_tex, TexCoords).rgb;
    vec3 val_stroke = texture(stroke_tex, TexCoords).rgb;
    vec3 val_mask = texture(mask_tex, TexCoords).rgb;

    if(val_mask.r>0)
    {
        final_val=1.0f;

        if(val_infill.r<0.001)
        {
            final_val=0.f;            
        }

        if(val_stroke.r>0)
        {
            final_val=1.f;            
        }

    }

    
    FragColor=vec4(final_val,final_val,final_val,1.0);
   
        
    

    
} 