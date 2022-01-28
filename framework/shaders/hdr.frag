#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform bool hdr;
uniform int hdr_algo_selection;
uniform float exposure;

// => Hable tmo
vec3 tm_partial(vec3 x)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x + C*B) + D*E)/(x*(A*x + B) + D*F)) - E/F;
}

// => UnrealEngine4 tmo
vec3 mul(mat3 m, vec3 v)
{
    float x = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2];
    float y = m[1][0] * v[1] + m[1][1] * v[1] + m[1][2] * v[2];
    float z = m[2][0] * v[1] + m[2][1] * v[1] + m[2][2] * v[2];
    return vec3(x, y, z);
}

vec3 rtt_and_odt_fit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}


void main()
{             
    const float gamma = 2.2;
    const float c_max = 0.5;
    vec3 c = texture(hdrBuffer, TexCoords).rgb;
    vec3 result = vec3(1.0);

    if(hdr)
    {
        // Global tone mapping
        // simple radiance
        if(hdr_algo_selection == 1)
        {
            result = c/(c + 1.0);
        }
        // extended radiance
        // uses custom white point
        // => overly bright in example
        // TODO : fetch c_max from texture
        if(hdr_algo_selection == 2)
        {
            result = (c*(1.0 + (c/pow(c_max,2))))/(c + 1.0);
        }

        // simple reinhard (luminance)
        // => orange tint
        if(hdr_algo_selection == 3)
        {
            float l_in = dot(c, vec3(0.2126f, 0.7152f, 0.0722f));
            result = c/(1.0 + l_in);
        }
        // modified reinhard 
        // combines work on rgb color and luminance
        // => less orange tint
        if(hdr_algo_selection == 4)
        {
            // convert rga color to luminance
            float l_in = dot(c, vec3(0.2126f, 0.7152f, 0.0722f));
            vec3 c_mod = c/(vec3(1.0) + c);
            // mix - returns linear interpolation of two scalars or vectors based on a weight
            result = mix(c/(1.0 + l_in), c_mod, c_mod);
        }

        // exposure
        if(hdr_algo_selection == 5)
        {
            result = vec3(1.0) - exp(-c*exposure);
        }

        // Hable tmo (Uncharted2)
        if(hdr_algo_selection == 6)
        {
            float exposure_bias = 2.0;
            vec3 curr = tm_partial(c*exposure_bias);
            vec3 W = vec3(11.2);
            vec3 white_scale = vec3(1.0) / tm_partial(W);
            result = curr*white_scale;
        }

        // UnrealEngine4 tmo
        if(hdr_algo_selection == 7)
        {
            mat3 aces_input_matrix;
            aces_input_matrix[0] = vec3(0.59719f, 0.35458f, 0.04823f);
            aces_input_matrix[1] = vec3(0.07600f, 0.90834f, 0.01566f),
            aces_input_matrix[2] = vec3(0.02840f, 0.13383f, 0.83777f);
            mat3 aces_output_matrix;
            aces_output_matrix[0] = vec3( 1.60475f, -0.53108f, -0.07367f);
            aces_output_matrix[1] = vec3(-0.10208f,  1.10813f, -0.00605f);
            aces_output_matrix[2] = vec3(-0.00327f, -0.07276f,  1.07602f);
            
            c = mul(aces_input_matrix, c);
            c = rtt_and_odt_fit(c);
            result = mul(aces_output_matrix, c);
        }
        

        // gamma correction
        result = pow(result, vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
    }
    else
    {
        // gamma correct but with unaltered color
        vec3 result = pow(c, vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
    }
}