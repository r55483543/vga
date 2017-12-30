#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>

//14byte文件头
typedef struct
{
	char cfType[2];  //文件类型，"BM"(0x4D42)
	int cfSize;     //文件大小（字节）
	short cfReserved; //保留，值为0
	short cf2Reserved; //保留，值为0
	int cfoffBits;  //数据区相对于文件头的偏移量（字节）
}__attribute__((packed)) BITMAPFILEHEADER;
//__attribute__((packed))的作用是告诉编译器取消结构在编译过程中的优化对齐

//40byte信息头
typedef struct {
	int ciSize;       //BITMAPFILEHEADER所占的字节数
	int ciWidth;         //宽度
	int ciHeight;        //高度
	char ciPlanes[2];     //目标设备的位平面数，值为1
	short ciBitCount;       //每个像素的位数
	char ciCompress[4];   //压缩说明
	char ciSizeImage[4];  //用字节表示的图像大小，该数据必须是4的倍数
	char ciXPelsPerMeter[4];//目标设备的水平像素数/米
	char ciYPelsPerMeter[4];//目标设备的垂直像素数/米
	char ciClrUsed[4];      //位图使用调色板的颜色数
	char ciClrImportant[4]; //指定重要的颜色数，当该域的值等于颜色数时（或者等于0时），表示所有颜色都一样重要
}__attribute__((packed)) BITMAPINFOHEADER;

typedef struct {
	//24bpp, RGB888
	unsigned char blue;
	unsigned char green;
	unsigned char red;
}__attribute__((packed)) PIXEL;//颜色模式RGB

BITMAPFILEHEADER FileHead;
BITMAPINFOHEADER InfoHead;

static char *fbp = 0;

static  int showbmp(struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
{
	FILE *fp;
	int rc;
	int line_x, line_y;
	long int location = 0, BytesPerLine = 0;
	char tmp[1024*10];
	printf("sizeof(long) = %d\n",sizeof(long));
	printf("sizeof(int) = %d\n",sizeof(int));
	printf("sizeof(short) = %d\n",sizeof(short));
	printf("sizeof(unsigned char) = %d\n",sizeof(unsigned char));
	if(!vinfo || !finfo)
		return -EINVAL;
	
	fp = fopen( "lenna.bmp", "rb" );
	if(!fp) {
		return -1;
	}

	rc = fread(&FileHead, sizeof(BITMAPFILEHEADER), 1, fp);
	if(rc != 1) {
		printf("read header error!\n");
		fclose(fp);
		return -2;
	}

	//检测是否是bmp图像
	if(memcmp(FileHead.cfType, "BM", 2) != 0) {
		printf("it's not a BMP file\n");
		fclose(fp);
		return -3;
	}

	printf("file info:\n"
			"  file size: %u\n"
			"  data offset: %u\n",
			FileHead.cfSize,
			FileHead.cfoffBits);
	
	rc = fread((char *)&InfoHead, sizeof(BITMAPINFOHEADER), 1, fp);
	if( rc != 1) {
		printf("read infoheader error!\n");
		fclose(fp);
		return -4;
	}
	
	printf("header info:\n"
			"  info size: %u\n"
			"  width x height: %d x %d\n"
			"  bpp: %d\n",
			InfoHead.ciSize,
			InfoHead.ciWidth, InfoHead.ciHeight,
			InfoHead.ciBitCount);

	//跳转的数据区
	fseek(fp, FileHead.cfoffBits, SEEK_SET);
	
	printf("sizeof(PIXEL)=%d\n", sizeof(PIXEL));
	
	line_x = line_y = 0;
	//向framebuffer中写BMP图片
	while(!feof(fp))
	{
		PIXEL pix;
		unsigned short int tmp;
		
		rc = fread( (char *)&pix, 1, sizeof(PIXEL), fp);
		if(rc != sizeof(PIXEL))
			break;
		
		//location = line_x * vinfo->bits_per_pixel / 8 + line_y * finfo->line_length; //上下翻转了
		location = line_x * vinfo->bits_per_pixel / 8 + (InfoHead.ciHeight - line_y -1) * finfo->line_length;//矫正
	
		//显示每一个像素
		*(fbp + location + 0) = pix.blue;
		*(fbp + location + 1) = pix.green;
		*(fbp + location + 2) = pix.red;
		*(fbp + location + 3) = 0;
	
		line_x++;
		if(line_x == InfoHead.ciWidth) {
			line_x = 0;
			line_y++;
			if(line_y == InfoHead.ciHeight)
				break;
		}
	}
	
	fclose(fp);
	return 0;
}

int main()
{
	int fd=0;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long screensize=0;
	int x = 0, y = 0;
	long location = 0;
	int i = 0;

	fd = open ("/dev/fb0",O_RDWR);
	if(fd < 0) {
		perror("open /dev/fb0:");
		goto end;
	}

	if(ioctl(fd, FBIOGET_FSCREENINFO, &finfo) != 0) {
	 	fprintf(stderr, "Error reading fixed information/n");
	 	goto fb_close;
	}
	 
	if(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) != 0) {
	 	fprintf(stderr, "Error reading variable information/n");
	 	goto fb_close;
	}

	printf("The mem is :%d\n", finfo.smem_len);
	printf("The line_length is :%d\n", finfo.line_length);
	printf("The xres is :%d\n", vinfo.xres);
	printf("The yres is :%d\n", vinfo.yres);
	printf("bits_per_pixel is :%d\n", vinfo.bits_per_pixel);

	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	fbp =(char *) mmap (0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
	if ((int) fbp == -1) {
		fprintf(stderr, "Error: failed to map framebuffer device to memory./n");
		goto fb_close;
	}
#if 0
	for(y=0; y<300; y++) {
		for(x=0; x<300; x++) {
			location = x * (vinfo.bits_per_pixel/8) + y * finfo.line_length;
			*(fbp + location) = 100;
	 		*(fbp + location + 1) = 15 + (x -100) / 2;
	 		*(fbp + location + 2) = 200 - (y - 100) / 5;  
	 		*(fbp + location + 3) = 250;
		}
	}
	sleep(1);
	for(y=100; y<200; y++) {
		for(x=0; x<300; x++) {
			location = x * (vinfo.bits_per_pixel/8) + y * finfo.line_length;
			*(fbp + location) = 0;
	 		*(fbp + location + 1) = 0;
	 		*(fbp + location + 2) = 0;  
	 		*(fbp + location + 3) = 250;
		}
	}
#endif
	showbmp(&vinfo, &finfo);

	munmap (fbp, screensize);

fb_close:
	 close (fd);
end:
	return 0;
}