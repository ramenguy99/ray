#pragma pack(push, 1)
struct bitmap_info_header
{
    u32 biSize;
    s32 biWidth;
    s32 biHeight;
    u16 biPlanes;
    u16 biBitCount;
    u32 biCompression;
    u32 biSizeImage;
    s32 biXPelsPerMeter;
    s32 biYPelsPerMeter;
    u32 biClrUsed;
    u32 biClrImportant;
};

struct bitmap_file_header {
    u16 bfType;
    u32 bfSize;
    u16 bfReserved1;
    u16 bfReserved2;
    u32 bfOffBits;
};
#pragma pack(pop)

struct image_data
{
    s32 Width;
    s32 Height;
    s32 Pitch;
    s32 BytesPerPixel;
    
    u8* Data;
};


internal u32
GetRGBAPixel(image_data* Image, s32 x, s32 y)
{
    u32 Result = *(u32*)(Image->Data + Image->BytesPerPixel * x + Image->Pitch *  y);
    return Result;
}

internal image_data
AllocateImage(u32 Width, u32 Height, u32 BytesPerPixel = 4)
{
    image_data Result;
    Result.Width = Width;
    Result.Height = Height;
    Result.BytesPerPixel = BytesPerPixel;
    Result.Pitch = Width * BytesPerPixel;
    Result.Data = (u8*)ZeroAlloc(Width * Height * BytesPerPixel);
    For(y, Height)
    {
        For(x, Width)
        {
            *(((u32*)Result.Data) + x + y * Width) = 0xFF000000;
        }
    }
    
    return Result;
}

internal void
WriteImageToBMPFile(image_data* Image, char* FileName)
{
    bitmap_info_header Header = {};
    Header.biSize = sizeof(Header);
    Header.biWidth = Image->Width;
    Header.biHeight = Image->Height;
    Header.biPlanes = 1;
    Header.biBitCount = 24; //RGB 24 bits per pixel
    Header.biCompression = 0; //BI_RGB
    
    //Looks like these fields can be left zero
    //    Header.biSizeImage = ALIGN_UP(Image->Width * 3, 4) * Image->Height; //Include padding to 4 bytes alignment for each row
    //    Header.biXPelsPerMeter = 2835; //72 DPI
    //    Header.biYPelsPerMeter = 2835; //72 DPI
    
    Header.biClrUsed = 0;
    Header.biClrImportant = 0;
    
    bitmap_file_header FileHeader = {};
    FileHeader.bfType = 0x4D42; //BM
    FileHeader.bfSize = sizeof(FileHeader) + sizeof(Header) + Header.biSize;
    FileHeader.bfOffBits = sizeof(FileHeader) + sizeof(Header);
    
    FILE* File = fopen(FileName, "wb+");
    fwrite(&FileHeader, sizeof(FileHeader), 1, File);
    fwrite(&Header, sizeof(Header), 1, File);
    
    u32 AlignmentBytes = ALIGN_UP(Image->Width * 3, 4) - Image->Width * 3;
    u32 Zero = 0;
    
    for(s32 y = Image->Height - 1; y >= 0; y--)
    {
        for(s32 x = 0; x < Image->Width; x++)
        {
            u32 Pixel = ((u32*)Image->Data)[y * Image->Width + x];
            Pixel = RGBToBGR(Pixel);
            fwrite(&Pixel, 3, 1, File);
        }
        
        fwrite(&Zero, AlignmentBytes, 1, File);
    }
}

