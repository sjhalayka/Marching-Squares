#include "image.h"

float int_rgb_to_float_grayscale(const unsigned char r, const unsigned char g, const unsigned char b)
{
	// http://www.itu.int/rec/R-REC-BT.709/en
	return	  0.2126f*(static_cast<float>(r) / 255.0f)\
			+ 0.7152f*(static_cast<float>(g) / 255.0f)\
			+ 0.0722f*(static_cast<float>(b) / 255.0f);
}

bool convert_tga_to_float_grayscale(const char *const filename, tga &t, float_grayscale &l, const bool make_black_border, const bool reverse_rows, const bool reverse_pixel_byte_order)
{
	// http://local.wasp.uwa.edu.au/~pbourke/dataformats/tga/
	ifstream in(filename, ios::binary);

	if(!in.is_open())
	{
		cerr << "Failed to open TGA file: " << filename << endl;
		return false;
	}

	// Read in header, including variable length image descriptor.
	in.read(reinterpret_cast<char *>(&t.idlength), 1);
	in.read(reinterpret_cast<char *>(&t.colourmaptype), 1);
	in.read(reinterpret_cast<char *>(&t.datatypecode), 1);
	in.read(reinterpret_cast<char *>(&t.colourmaporigin), 2);
	in.read(reinterpret_cast<char *>(&t.colourmaplength), 2);
	in.read(reinterpret_cast<char *>(&t.colourmapdepth), 1);
	in.read(reinterpret_cast<char *>(&t.x_origin), 2);
	in.read(reinterpret_cast<char *>(&t.y_origin), 2);
	in.read(reinterpret_cast<char *>(&t.px), 2);
	in.read(reinterpret_cast<char *>(&t.py), 2);
	in.read(reinterpret_cast<char *>(&t.bitsperpixel), 1);
	in.read(reinterpret_cast<char *>(&t.imagedescriptor), 1);

	if(0 != t.idlength)
	{
		t.idstring.resize(t.idlength + 1, '\0'); // Terminate this ``C style'' string properly.
		in.read(&t.idstring[0], t.idlength);
	}

	// Read pixels, convert to floating point.
	if(2 != t.datatypecode)
	{
		cerr << "TGA file must be in uncompressed/non-RLE 24-bit RGB format." << endl;
		return false;
	}
	else
	{
		if(24 != t.bitsperpixel)
		{
			cerr << "TGA file must be in uncompressed/non-RLE 24-bit RGB format." << endl;
			return false;
		}

		// Read all pixels at once.
		size_t num_bytes = t.px*t.py*3;
		t.pixel_data.resize(num_bytes);
		in.read(reinterpret_cast<char *>(&t.pixel_data[0]), num_bytes);

		if(true == reverse_rows)
		{
			// Reverse row order.
			short unsigned int num_rows_to_swap = t.py;
			vector<unsigned char> buffer(t.px*3);

			if(0 != t.py % 2)
				num_rows_to_swap--;

			num_rows_to_swap /= 2;

			for(short unsigned int i = 0; i < num_rows_to_swap; i++)
			{
				size_t y_first = i*t.px*3;
				size_t y_last = (t.py - 1 - i)*t.px*3;

				memcpy(&buffer[0], &t.pixel_data[y_first], t.px*3);
				memcpy(&t.pixel_data[y_first], &t.pixel_data[y_last], t.px*3);
				memcpy(&t.pixel_data[y_last], &buffer[0], t.px*3);
			}
		}

		if(true == make_black_border)
		{
			// Make border pixels black.
			for(unsigned short int x = 0; x < t.px; x++)
			{
				for(unsigned short int y = 0; y < t.py; y++)
				{
					if(x == 0 || x == t.px - 1 || y == 0 || y == t.py - 1)
					{
						size_t index = y*t.px*3 + x*3;
						t.pixel_data[index] = 0;
						t.pixel_data[index + 1] = 0;
						t.pixel_data[index + 2] = 0;
					}
				}
			}
		}

		// Fill floating point grayscale image.
		l.px = t.px;
		l.py = t.py;
		l.pixel_data.resize(num_bytes/3, 0);

		for(size_t index = 0; index < num_bytes; index += 3)
		{
			if(reverse_pixel_byte_order)
			{
				// Swap red and blue pixels.
				unsigned char temp = t.pixel_data[index];
				t.pixel_data[index] = t.pixel_data[index + 2];
				t.pixel_data[index + 2] = temp;
			}

			// Convert to luma.
			l.pixel_data[index/3] = int_rgb_to_float_grayscale(t.pixel_data[index], t.pixel_data[index + 1], t.pixel_data[index + 2]);
		}
	}

	return true;
}
