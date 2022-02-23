#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

const int box_size=20;


void main()
{
    vec3 col = texture(screenTexture, TexCoords).rgb;

    float r_value=col.r;
    float is_skin=0.0f;

    //vec2 frag_coord=gl_FragCoord.xy-vec2(0.5);

    int half_box_size=box_size/2;

    // must inside the polygon
    if(r_value>0.001)
    {
        vec2 tex_coord_shift;
        for(int i =0;i <box_size; i++)
        {
            for(int j =0;j <box_size; j++)
            {
                
                vec2 texcoord_new=TexCoords+vec2((i-half_box_size)*1.0/1024.f,(j-half_box_size)*1.0/1024.f);
                
                // outside 0-1 do nothing
                if((texcoord_new.x<0)||(texcoord_new.x>0.9999)||(texcoord_new.y<0)||(texcoord_new.y>0.999))
                {

                }
                else
                {
                    if(texture(screenTexture, texcoord_new).r<0.01)
                    {
                        is_skin=r_value;
                    }
                }
                
                
            }
        }

    }
    

    float grayscale = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;


    //FragColor = vec4(is_skin,col.g,0, 1.0);
    FragColor=vec4(col,1.0);
} 