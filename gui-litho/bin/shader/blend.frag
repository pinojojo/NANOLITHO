#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D maskTexture;
uniform sampler2D infillTexture;
uniform sampler2D strokeTexture;





void main()
{
    
    
    vec3 col_mask = texture(maskTexture, TexCoords).rgb;
    vec3 col_infill = texture(infillTexture, TexCoords).rgb;
    vec3 col_stroke = texture(strokeTexture, TexCoords).rgb;
    
    FragColor = vec4(0,0,0, 1.0);

    // Intersection operations : find minimum value of above

    float final_val=0;
    
    

    if(col_mask.r>0.01)
    {
        final_val=col_mask.r;
        if(col_infill.r<0.01){final_val=0;}
        if(col_stroke.r>0.01)
        {
            if(col_mask.r<col_stroke.r)
            {
                final_val=col_mask.r;
            }else
            {
                final_val=1;
            }

        }
    }

   



   FragColor=vec4(final_val,final_val,final_val,1.0);
     //FragColor=vec4(col_stroke.r,0.0,0.0,1.0);
        
    

    
} 