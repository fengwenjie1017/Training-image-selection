// Training Image Selection.cpp : Defines the entry point for the console application.
//
//#include "stdafx.h"
#include <ctime>
#include "omp.h"
#include <afx.h>
#include <windows.h>
#include <iostream>
#include <afx.h>
#include <io.h>
#include <fstream>
#include <string>
#include <time.h>
#include <stdio.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace std; 


// Grid data structure
struct GridV
{
  int discprop;    // 模拟网格中的离散变量
  double contprop;     //模拟网格中的连续变量
  int iscddata;   // 是否条件数据
};

//Structure of random path
struct RandomRoutine
{
  int rpx;
  int rpy;
  int rpz;
};

//Structure of geological patterns
struct GeoPattern
{
	int px;
	int py;
	int pz;
	int facies;
	double ContVariable;   //连续变量
};

//Matched point in the TI
struct MatchedPoints
{
	int MPx;
	int MPy;
	int MPz;
};

//-----------------------------------------------------------------
//                Parameters Definition
//-----------------------------------------------------------------
// 调试用参数
   int nED_0total = 0;  //ED=0的数据事件总数
// 距离计算参数定义
   int DistanceType=0;    //匹配度(数据事件的距离)计算方式（0-简单模式；1-基于滞后距加权[cate]；2-欧氏距离[cont]），下一行参数根据本项选择而定
// Grid definition
   int Nx,Ny,Nz;      //number of nodes
   CString Datafl=" ";    //conditional data file path
   int NTI=0;    //训练图像的数量
   int TIType=0;    //训练图像类型
// CString *TrainingImageFl;  //Training Image file path
   string *TrainingImageFl;  //Training Image file path   
   string *TemplateFl;  //Training Image file path   
// Training Image size
   int TINx,TINy,TINz;     //单一训练图像的规模，用于全局
   int *TIscalex,*TIscaley,*TIscalez;    //读取参数中多个训练图像的规模，用于存储
   int *NTemplateNode;  //每个tenplate的节点数
   int ***TemplateNode;   //template节点坐标
// Allocatable Arrays
   GridV ***GridValue;      //模拟网格对象数组
   int ****Replication;      //模拟重复数数组
   double *****MatchingDegree;    //用于保存比对得到的多个尺度的匹配度数据
   int ***TIValue;    //离散型三维训练图像动态数组
   double ***ContTIValue;    //连续型变量训练图像动态数组
// data patterns definition
   int MaxNode=100;   //最多MaxNode个点参与模式搜索，即灵活样板最多容纳的条件数据点个数。
//   int PatternNode;  //Number of nodes within a pattern，assigned by user
// User preference settings
   int CheckPf=1;   //check Parameter file while reading 
   int seed=69069;  // random seed(to read from the parameter file)
//   int searchwindowx=1,searchwindowy=1,searchwindowz=1;  //搜索窗口的大小,初始设置为10*10*1。
   GeoPattern *GP;  //GeoPattern
   int MinCDNum=0;    //每个地质模式中最小的条件数据点数，达不到这个数目，不创建地质模式，寻求其他的方式解决
   int NCD=0;  //搜索窗内条件数据点的个数；
// Random routine structure array
   RandomRoutine *RR; 
   double MinD=0;  //在训练图像中进行模式搜索时，可进行匹配度评估要求的最小条件数据点数比例
   double MinMatchingDegree=0.0;  //模拟需要的最小匹配度（GP与获取的数据事件）
   MatchedPoints mp;   //每次模拟中，选中的数据事件在TI中的位置
   int Ncut=0;   //离散变量的类型数
   int match=0;   //在搜寻数据事件时，是否找到匹配的数据事件
   int DisplayDetails=0;   //是否显示模拟过程中的具体细节（1-显示；0-不显示）
   int DisplaySearch_Match = 0;   //是否显示模拟过程中的地质模式的搜索和匹配信息（1-显示；0-不显示）
   int NLevel=1;    //数据事件匹配分级数量，默认为1级
   int **searchwindowscale;   //用于多尺度搜索的搜索窗口大小
   int iLevel=0;    //指示当前正在进行计算的尺度编号，全局变量，默认为0，即最小尺度。
   CString MatchingResultfl;    //匹配比对结果输出文件路径
   int GPmatching=0;    //当前是否进行GP的匹配-取决于GP是否被创建，0-否，1-是
   int iTI=0;   //多训练图像的循环,全局变量
   int DisplayException=0;    //显示匹配过程中的异常GP
   double CalRatio=0.0;   //参与计算的比率
//  int **ND;  //获取的条件数据事件数量,用于计算平均获取的数据点数
//  double **NDAVG;  //获取的条件数据事件数量,用于计算平均获取的数据点数平均值
   int ConsideredCellNum=0;   //参与计算的网格点数
   int ReadPath=0;   //是否读取随机路径
   int NTIData=0;    //用于训练图像读取
   int **NumInvalidGP;  //每幅训练图像中，不同搜索样板条件下，无法获取有效的地质模式的点数
   int **NumNotEnoughNodesinTI;   //在每幅训练图像中，不同搜索样板条件下，无法在训练图像中获取具有足够多网格点的数据事件的网格点数
   int **NumNotEnoughNodesinTIGP;  //在每幅训练图像中，不同搜索样板条件下，获取到足够多的网格点，但少于参数要求的网格点数(NCD/2)
   int NumCDGP = 0;  //用于检查条件数据点处是否有创建GP
   int TemplateType = 0;  //搜索样板类型 template type

//-----------------------------------------------------------------
//               播放提示音并等待5秒
//-----------------------------------------------------------------
void PlayEmergencySound(int times)
{
	for (int i = 0; i < times; i++)
   {
	   PlaySound(TEXT("BEEP1.WAV"), NULL, SND_FILENAME);
   }
   Sleep(5000);
}

//-----------------------------------------------------------------
//                   播放提示音
//-----------------------------------------------------------------
void PlayTipsSound(int times)
{
	for (int i = 0; i < times; i++)
	{
		PlaySound(TEXT("Alarm01.wav"), NULL, SND_FILENAME);
	}
	//Sleep(1000);
}

//-----------------------------------------------------------------
//               Input Tips Messages
//-----------------------------------------------------------------
void GreenTipsMsg(CString InputTipsMsg)
{
    //Get the color set handle
	HANDLE hOut;
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);  
	//Set the color
    SetConsoleTextAttribute(hOut,  
                            BACKGROUND_GREEN |
							BACKGROUND_INTENSITY);  
    cout <<InputTipsMsg;//<<endl; 
	//set the color back
	SetConsoleTextAttribute(hOut,  
                            FOREGROUND_GREEN |  
                            FOREGROUND_INTENSITY); 
	cout<<endl;
}

//-----------------------------------------------------------------
//              allocate MatchingDegree arrays
//-----------------------------------------------------------------
void AllocateMatchingDegreeArray()
{
	//循环变量 
	int i,j,k,m,n;
	//allocate arrays
	MatchingDegree=new double ****[NTI];
	for(n=0;n<NTI;n++)
	{
		MatchingDegree[n]=new double***[NLevel];    //针对不同相的重复数，类似于E-type Map
		for(i=0;i<NLevel;i++)
		{
			MatchingDegree[n][i]=new double**[Nz];
			for(j=0;j<Nz;j++) 
			{
				MatchingDegree[n][i][j]=new double*[Ny];
				for(m=0;m<Ny;m++)
				{
					MatchingDegree[n][i][j][m]=new double[Nx];
				}
			}
		}
	}
	
	for(n=0;n<NTI;n++)
		for (m=0;m<NLevel;m++)
			for(i=0;i<Nz;i++)
				for(j=0;j<Ny;j++)
					for(k=0;k<Nx;k++)
					{
						MatchingDegree[n][m][i][j][k]=-999.00;
					}

	GreenTipsMsg("MatchingDegree array(4D) was initialed");
	system("pause");
}
//-----------------------------------------------------------------
//              allocate GridValue arrays
//-----------------------------------------------------------------
void AllocateGridValue()
{
    //循环变量 
    int i,j,k,m;
	//allocate arrays
	GridValue=new GridV**[Nz];     //网格值，包括条件数据和模拟值
	Replication=new int***[Nz];    //针对不同相的重复数，类似于E-type Map
		for(i=0;i<Nz;i++)
			{
				GridValue[i]=new GridV*[Ny];
				Replication[i]=new int **[Ny];
  				for(j=0;j<Ny;j++) 
					{
						GridValue[i][j]=new GridV[Nx];
						Replication[i][j]=new int*[Nx];
						for (m=0;m<Nx;m++)
						{
							Replication[i][j][m]=new int[Ncut];
						}
					}
			};

		for(i=0;i<Nz;i++)
			for(j=0;j<Ny;j++)
				for(k=0;k<Nx;k++)
				{
					GridValue[i][j][k].discprop=-999;
					GridValue[i][j][k].contprop=-999.00;
					GridValue[i][j][k].iscddata=-1;    //初始化为非条件数据
					for (m=0;m<Ncut;m++)
					{
						Replication[i][j][k][m]=0;
					}
				}
	GreenTipsMsg("GridValue was initialed");
	system("pause");
}

//-----------------------------------------------------------------
//               OutPut Warning Messages
//-----------------------------------------------------------------
void YellowWarnMsg(CString WarnMsg)
{
    //Get the color set handle
	HANDLE hOut;
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);  
	//Set the color
    SetConsoleTextAttribute(hOut,  
                            BACKGROUND_RED |
							BACKGROUND_GREEN |
							BACKGROUND_INTENSITY
                            );//  
    cout <<WarnMsg; 
	//set the color back
	SetConsoleTextAttribute(hOut,  
                            FOREGROUND_GREEN |  
                            FOREGROUND_INTENSITY);
	cout<<endl;
}

//-----------------------------------------------------------------
//                Error Tips Messages
//-----------------------------------------------------------------
void OutputErrorTipsMsg(CString ErrorTipsMsg)
{
    //Get the color set handle
	HANDLE hOut;
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);  
	//Set the color
    SetConsoleTextAttribute(hOut,  
                            BACKGROUND_RED |
							BACKGROUND_INTENSITY);  
    cout <<ErrorTipsMsg; 
	//set the color back
	SetConsoleTextAttribute(hOut,  
                            FOREGROUND_GREEN |  
                            FOREGROUND_INTENSITY); 
	cout<<endl;
}

//-----------------------------------------------------------------
//         allocate Training Image Value arrays
//-----------------------------------------------------------------
void AllocateTrainingImageValue()
{
    //循环变量 
    int i,j,k;
	//allocate arrays
	TIValue=new int**[TINz];
		for(i=0;i<TINz;i++)
		{
			TIValue[i]=new int*[TINy];
			for(j=0;j<TINy;j++) 
				TIValue[i][j]=new int[TINx];
		};
	//初始化为-99
	for(i=0;i<TINz;i++)
		for(j=0;j<TINy;j++)
			for(k=0;k<TINx;k++)
			{
				TIValue[i][j][k]=-99;
			}
	GreenTipsMsg("Training Image Value Array was initialed");
}

//-----------------------------------------------------------------
//         allocate Training Image Value arrays
//-----------------------------------------------------------------
void DeAllocateTrainingImageValue(int TInx, int TIny, int TInz)
{
	//循环变量 
	cout<<"START TO DEALLOCATE!!!!!!!!!!!!!!!!!!!!!"<<endl;
	int i,j;
	//deallocate arrays
	if (TIType==0)
	{
		for (i = 0; i<TInz; i++)
		{
			for (j = 0; j<TIny; j++)
				delete[] TIValue[i][j];
		}
		for (i = 0; i<TInz; i++)
			delete[] TIValue[i];
		delete[] TIValue;
		TIValue=NULL;
	}
	if (TIType == 1)
	{
		for (i = 0; i < TInz; i++)
		{
			for (j = 0; j < TIny; j++)
				delete[] ContTIValue[i][j];
		}
		for (i = 0; i < TInz; i++)
			delete[] ContTIValue[i];
		delete[] ContTIValue;
		ContTIValue = NULL;
	}
	
	GreenTipsMsg("Training Image Value Array was deleted");
}

//-----------------------------------------------------------------
//         allocate Continual Training Image Value arrays
//-----------------------------------------------------------------
void AllocateContTrainingImageValue()
{
	//循环变量 
	int i,j,k;
	//allocate arrays
	ContTIValue=new double**[TINz];
	for(i=0;i<TINz;i++)
	{
		ContTIValue[i]=new double*[TINy];
		for(j=0;j<TINy;j++) 
			ContTIValue[i][j]=new double[TINx];
	};

	for(i=0;i<TINz;i++)
		for(j=0;j<TINy;j++)
			for(k=0;k<TINx;k++)
			{
				ContTIValue[i][j][k]=-99;
			}
	GreenTipsMsg("Continual Training Image Value Array was initialed");
}

//-----------------------------------------------------------------
//         deallocate continual Training Image Value arrays
//-----------------------------------------------------------------
void DeAllocateContTrainingImageValue(int m, int n, int p)
{
	//循环变量 
	int i,j;
	//deallocate arrays
	for(i=0;i<p;i++)
	{
		for(j=0;j<n;j++) 
			delete ContTIValue[i][j];
		delete ContTIValue[i];
	};
	delete ContTIValue;
	GreenTipsMsg("Training Image Value Array was deleted");
}

void AllocateAbnormalSituationArray()
{
	//循环变量
	int i = 0, j = 0, k = 0;
	//动态分配数组
	NumInvalidGP = new int* [NTI];
	NumNotEnoughNodesinTI = new int*[NTI];
	NumNotEnoughNodesinTIGP = new int*[NTI];
		for (i = 0; i < NTI;i++)
		{
			NumInvalidGP[i] = new int[NLevel];
			NumNotEnoughNodesinTI[i] = new int[NLevel];
			NumNotEnoughNodesinTIGP[i] = new int[NLevel];
		}
		for (i = 0; i < NTI; i++)
			for (j = 0; j < NLevel;j++)
			{
				NumInvalidGP[i][j] = 0;
				NumNotEnoughNodesinTI[i][j] = 0;
				NumNotEnoughNodesinTIGP[i][j] = 0;
			}
}

//-----------------------------------------------------------------
//              Read Training Image File
//-----------------------------------------------------------------
int readTI(string TIPath)
{
	errno_t err;
	//tips
	GreenTipsMsg("--->Start to READING Training Image file ");
	//File pointer
	FILE *TIf;
	//Number of data points at every step
	//data
	double v=0.0;
	//temple string
	char top[100];
	//Open and Read para file
	if ((err = fopen_s(&TIf, TIPath.c_str(), "r")) != 0)
	{
		OutputErrorTipsMsg("===TI data file Path Error!!!");
		return 0;
	}
	else
	{
		//Read file top
		fgets(top,99,TIf);
		fgets(top,99,TIf);
		fgets(top,99,TIf);
		NTIData=0;
		//读取训练图像，注意训练图像如果是petrel导出的，那么Z方向的顺序是由大到小
		for (int i = 0; i <TINz ; i++)
			for (int j = 0; j < TINy; j++)
				for (int k = 0; k < TINx; k++)
				{
					fscanf_s(TIf, "%lf", &v);
					if (TIType == 0)
					{
						TIValue[TINz-1-i][j][k] = int(v);
					}
					if (TIType == 1)
						ContTIValue[TINz - 1-i][j][k] = v;
					v = -999;
					NTIData++;
				}
	}
	fclose(TIf);
	cout<<"Training Image file reading completed!!!"<<endl;
	cout<<"Number of Training Image points:"<<NTIData<<endl;
	return 1;
}


//-----------------------------------------------------------------
//             Read Conditional data File
//-----------------------------------------------------------------
int readcond()
{
	errno_t err;
	//tips
	GreenTipsMsg("--->Start to READING Conditional data file ");
	//File pointer
	FILE *condf;
	//Number of data points
	int NCondData=0;
	//data
	int x=0,y=0,z=0;double v=0.0;
	//temple string
	char top[100];
	char *dataflstr;
	dataflstr=Datafl.GetBuffer(Datafl.GetLength());
	Datafl.ReleaseBuffer();

	//Open and Read para file
	if ((err = fopen_s(&condf, dataflstr, "r")) != 0)
	{
		cout<<"===Conditional data file Path Error!!!\n"<<endl;
		return 0;
	}
	else
	{
		//Read file top
		fgets(top,99,condf);
		fgets(top,99,condf);
		fgets(top,99,condf);
		fgets(top,99,condf);
		fgets(top,99,condf);
		fgets(top,99,condf);
		//read conditional data
		while(fscanf_s(condf,"%d %d %d %lf",&x,&y,&z,&v)!=EOF)
		{
			//cout<<"x-"<<x<<"  y-"<<y<<"  z-"<<z<<"  v-"<<v<<endl;
			if (TIType==0)   //离散变量
			{
				GridValue[z-1][y-1][x-1].discprop=int(v);
			}
			if (TIType==1)  //连续型变量
			{
				GridValue[z-1][y-1][x-1].contprop=v;
			}
			//cout<<int(v)<<"/";   //用于检查条件数据
			GridValue[z-1][y-1][x-1].iscddata=1;  //表明该数据为条件数据
			NCondData++;
			//cout<<"Value:"<<GridValue[Nz-z][y-1][x-1].contprop<<"     order="<<NCondData<<endl;
		}
	}
	fclose(condf);
	cout<<"Conditional data reading completed!!!\n"<<"Number of data points:"<<NCondData<<"     Conditional data Type:"<<TIType<<endl;
	return 1;
}

//-----------------------------------------------------------------
//             Read template Files
//-----------------------------------------------------------------
int Readtemplates()
{
	errno_t err;
	//tips
	GreenTipsMsg("--->Start to READING Template files ");
	//Number of data points
	int NCondData = 0;
	//最大的样板点数
	int MaxPointNum=0;
	//data
	int x = 0, y = 0, z = 0;
	int x1 = 0, y1 = 0, z1 = 0;
	int i = 0,j=0;
	
	for (int nn = 0; nn < NLevel;nn++)
	{
		//File pointer
		FILE *tempf;
		//temple string
		char top[100];
		//Open and Read para file
		if ((err = fopen_s(&tempf, TemplateFl[nn].c_str(), "r")) != 0)
		{
			cout << "===template data file Path Error!!!\n" << endl;
			return 0;
		}
		else
		{
			//Read file top
			fgets(top, 99, tempf);
			fgets(top, 99, tempf);
			fgets(top, 99, tempf);
			fgets(top, 99, tempf);
			fgets(top, 99, tempf);
			//read template data
			while (fscanf_s(tempf, "%d %d %d", &x, &y, &z) != EOF)
			{
				x1 = x; y1 = y; z1 = z;
				NCondData++;
				//cout<<"Value:"<<GridValue[Nz-z][y-1][x-1].contprop<<"     order="<<NCondData<<endl;
			}
		}
		fclose(tempf);
		NTemplateNode[nn] = NCondData;
		if (MaxPointNum<NCondData)
		{
			MaxPointNum = NCondData;
		}
		NCondData = 0;
	}
	//allocate the template
	TemplateNode = new int**[NLevel];
	for (i = 0; i < NLevel; i++)
	{
		TemplateNode[i] = new int*[MaxPointNum];
		for (j = 0; j < MaxPointNum;j++)
		{
			TemplateNode[i][j] = new int[3];
		}
	}
	for (i = 0; i < NLevel; i++)
		for (j = 0; j < MaxPointNum;j++)
		{
			TemplateNode[i][j][0] = 0; TemplateNode[i][j][1] = 0; TemplateNode[i][j][2] = 0;
		}

	for (int nn = 0; nn < NLevel; nn++)
	{
		//File pointer
		FILE *tempf;
		//temple string
		char top[100];
		//Open and Read para file
		if ((err = fopen_s(&tempf, TemplateFl[nn].c_str(), "r")) != 0)
		{
			cout << "===template data file Path Error!!!\n" << endl;
			return 0;
		}
		else
		{
			//Read file top
			fgets(top, 99, tempf);
			fgets(top, 99, tempf);
			fgets(top, 99, tempf);
			fgets(top, 99, tempf);
			fgets(top, 99, tempf);
			//read template data
			while (fscanf_s(tempf, "%d %d %d", &x, &y, &z) != EOF)
			{
				TemplateNode[nn][NCondData][0] = x; TemplateNode[nn][NCondData][1] = y; TemplateNode[nn][NCondData][2] = z;
				NCondData++;
			}
		}
		fclose(tempf);
		NCondData = 0;
	}

	cout << "Reading Templates completed!!!\n" << endl;
	return 1;
}

//-----------------------------------------------------------------
//                 Read Parameter File
//-----------------------------------------------------------------
int ReadPar(CString ParaFile)
{
	int i=0,j=0;  //动态数组分配与初始化，循环变量
	errno_t err;
	ifstream fin(ParaFile);
	//tips
	GreenTipsMsg("--->Start to READING ParaFl ");
    //File pointer
	FILE *pf;
	//temple cstring
	char temp[100];
	//Open and Read para file
	if((err=fopen_s(&pf,ParaFile,"r"))!=0)
	{
		cout<<"===ParaFile Path Error!!!\n"<<endl;
		return 0;
	}
	else
	{
		//参与计算的网格单元比例
		fin>>CalRatio; fin.ignore(256,'\n');
		if(CheckPf) cout<<"Ratio of grid nodes to be considered:   "<<CalRatio<<endl;
		//训练图像类型
		fin>>TIType;  fin.ignore(256,'\n');
		if(CheckPf) cout<<"Training Image Type:   "<<TIType<<endl;
		//离散变量的类型数：Ncut
		fin>>Ncut;  fin.ignore(256,'\n');
		if(CheckPf) cout<<"Number of Category variables:   "<<Ncut<<endl;
		//匹配度计算的尺度分级数
		fin>>NLevel;  fin.ignore(256,'\n');
		if(CheckPf) cout<<"Number of templates:   "<<NLevel<<endl;
		//template文件路径数组动态分配与初始化
		TemplateFl = new string[NLevel];
		for (int m = 0; m < NLevel; m++)
			TemplateFl[m] = "";
		//template type
		fin >> TemplateType;  fin.ignore(256, '\n');
		if (CheckPf) cout << "Template type:   " << TemplateType << endl;
		//不同尺度的搜索窗口大小（动态分配与读取）
		searchwindowscale=new int*[NLevel];
		for(i=0;i<NLevel;i++)
			searchwindowscale[i]=new int[3];
		for(i=0;i<NLevel;i++)
			{
				for(j=0;j<3;j++)
				{
					searchwindowscale[i][j]=1;
					fin>>searchwindowscale[i][j];
				}
				if(CheckPf) cout<<"SearchWindow-x:"<<searchwindowscale[i][0]<<"   SearchWindow-y:"<<searchwindowscale[i][1]<<"   SearchwWindow-z:"<<searchwindowscale[i][2]<<endl;
				fin.ignore(256,'\n');
				//read the path of template file
				fin >> temp; TemplateFl[i] = temp;    fin.ignore(256, '\n');
				if (CheckPf) cout << "template file path:   " << temp << endl;
			}
		//匹配度(数据事件的距离)计算方式（0-简单模式；1-基于滞后距加权[cate]；2-欧氏距离[cont]），下一行参数根据本项选择而定
		fin>>DistanceType;   fin.ignore(256,'\n');
		if(CheckPf) cout<<"DistanceType:"<<DistanceType<<endl;
		//条件数据文件
		fin>>temp;Datafl=temp;    fin.ignore(256,'\n');
		if(CheckPf) cout<<"Conditional data file path:   "<<Datafl<<endl;
		//模拟网格大小
		fin>>Nx>>Ny>>Nz;   fin.ignore(256,'\n');
		if(CheckPf) cout<<"Nx:"<<Nx<<"   Ny:"<<Ny<<"   Nz:"<<Nz<<endl;
		//GridValue数组初始化
		AllocateGridValue();    //Allocate the GridValue array
		//训练图像数量
		fin>>NTI;  fin.ignore(256,'\n');
		if(CheckPf) cout<<"Number of TI:   "<<NTI<<endl;
		//判断训练图像类型是否合适
		if(DistanceType==2 && TIType!=1)
			OutputErrorTipsMsg("Training Image Type is conflicted with Distance type of Data Events");
		//训练图像文件路径数组动态分配与初始化
		TrainingImageFl=new string[NTI];
		for (int m=0;m<NTI;m++)
			TrainingImageFl[m]="";
		TIscalex=new int[NTI]; TIscaley=new int[NTI];  TIscalez=new int[NTI];
		//初始化
		for (i = 0; i < NTI; i++)
		{
			TIscalex[i] = 0; TIscaley[i] = 0; TIscalez[i] = 0;
		}
		for (i=0;i<NTI;i++)
		{
			//读取训练图像文件名
			fin>>temp;TrainingImageFl[i]=temp;   fin.ignore(256,'\n');
			//读取训练图像规模
			if(CheckPf) cout<<"File path of Training Image NO."<<i+1<<":"<<TrainingImageFl[i]<<endl;
			//读取训练图像的规模
			fin>>TIscalex[i]>>TIscaley[i]>>TIscalez[i];    fin.ignore(256,'\n');
			if(CheckPf) cout<<"Size of Training Image NO."<<i+1<<":"<<TIscalex[i]<<"*"<<TIscaley[i]<<"*"<<TIscalez[i]<<endl;
		}
		//MatchingDegree数组初始化
		AllocateMatchingDegreeArray();
		//异常记录数组初始化
		AllocateAbnormalSituationArray();
		//地质模式在训练图像中要求的最小匹配度
		fin>>MinMatchingDegree;    fin.ignore(256,'\n');
		if(CheckPf) cout<<"Min Matching Degree:   "<<MinMatchingDegree<<endl;

		//模式内点的数目----暂时未启用
		//fin>>PatternNode;    fin.ignore(256,'\n');
		//if(CheckPf) cout<<"PatternNode:"<<PatternNode<<endl;
		//If the parameter PatternNode is too large,warning and reset that parameter.
		//if(PatternNode>MaxNode) 
		//{
		//	YellowWarnMsg("Warning: PatternNode is larger than MaxNode !!!");
		//	PatternNode=MaxNode;
		//}
		//搜索窗内最小的条件数据点数：MinCDNum
		fin>>MinCDNum;   fin.ignore(256,'\n');
		if(CheckPf) cout<<"Min Conditional Data Points Number:   "<<MinCDNum<<endl;
		//在训练图像中进行模式搜索时，可进行匹配度评估要求的最小条件数据点数：MinD
		fin>>MinD;    fin.ignore(256,'\n');
		if(CheckPf) cout<<"Min Conditional Data Points Number:   "<<MinD<<endl;
		
		//随机种子
		fin>>seed;   fin.ignore(256,'\n');
		if(CheckPf) cout<<"Random seed:   "<<seed<<endl;
		//输出匹配度结果的文件路径
		fin>>temp; MatchingResultfl=temp;    fin.ignore(256,'\n');
		if(CheckPf) cout<<"Matching Results output file path:   "<<MatchingResultfl<<endl;
		//read templates
		if (TemplateType==1)
		{
			Readtemplates();
		}
	}
	fclose(pf);
	cout<<"Parameters reading completed!!!"<<endl;
	system("pause");
	return 1;
}

//-----------------------------------------------------------------
//              Write Version information
//-----------------------------------------------------------------
void WriteVersion()
{
	cout<<"***********************************************************\n";
	cout<<"              Training Image Selection Tools                    \n\n";
	cout<<"    by Wenjie Feng, Shenghe WU,Yanshu Yin, Ke Zhang \n";
	cout<<"                           V1.0\n";
	cout<<"                     Data: 2017-01-02\n";
	cout<<"***********************************************************\n"<<endl;
}

//-----------------------------------------------------------------
//     Read Parameter,Conditional data and TI file
//-----------------------------------------------------------------
int ReadParaAndData()
{
	//Parameter File operations
	string ParaFile="                          ";
	GreenTipsMsg("--->Input Parameter File Path: ");
	cin>>ParaFile;
	//Check the ParaFl
	if(_access(ParaFile.c_str(),0)==-1)
	{
		OutputErrorTipsMsg("Error: The ParaFl doesn't exist.check your parameter file path !!!");
		return 0;
	}
	
	//Switch to display details or not
	GreenTipsMsg("--->Display details?  Yes-1 , No-0");
	cin>>DisplayDetails;

	//选择显示或者不显示匹配中的异常GP
	YellowWarnMsg("--->Display exception of GP?  Yes-1 , No-0");
	cin>>DisplayException;

	//选择显示或者不显示匹配中的异常GP
	YellowWarnMsg("--->Display search and match process of GP?  Yes-1 , No-0");
	cin >> DisplaySearch_Match;

	//call the ParaFl reader and initial programs
	GreenTipsMsg("--->CHECK Parameter File?  Yes-1 , No-0 ");
	cin>>CheckPf; cout<<"CHECK ParaFl:   "<<CheckPf<<endl;
	if(ReadPar(ParaFile.c_str())==0)
	{
	 OutputErrorTipsMsg("===check your parameter file !!!");
	 return 0;
	}

	//Read the conditional data file with function "readcond()"
	if(readcond()==0)
	{
	 OutputErrorTipsMsg("===check your conditional data file path!!!");
	 return 0;
	}

	/*	//在主程序中读取训练图像
	//Read the Training Image file with function "readTI()"
	if(readTI()==0)
	{
	 OutputErrorTipsMsg("===check your Training Image file path!!!");
	 return 0;
	}
	*/
	//complete and return
    return 1;
}

//-----------------------------------------------------------------
//                       生成随机路径
//-----------------------------------------------------------------
void RandomPathConstruction(string RPfilename)
{
	errno_t err,err1;
	int i=0,j=0,k=0;
	int	***RandomNum;
	int maxnum=0;   //模拟网格数据点数
	maxnum=Nx*Ny*Nz;
	char top[100];
	int v1=0,v2=0,v3=0,v4=0;
	
	//排序中的最大值
	int maxrandomnum=0;   
	//随机分配随机路径数组并初始化
	RR=new RandomRoutine[maxnum];
	for(i=0;i<maxnum;i++)
	{
		RR[i].rpx=0; RR[i].rpy=0; RR[i].rpz=0;
	}
	//动态分配模拟数网格
	RandomNum=new int**[Nz];
	for(i=0;i<Nz;i++)
	{
		RandomNum[i]=new int*[Ny];
		for (j=0;j<Ny;j++)
		{
			RandomNum[i][j]=new int[Nx];
		}
	}
	//为每个模拟网格分配随机数
	for (i=0;i<Nz;i++)
		for(j=0;j<Ny;j++)
			for(k=0;k<Nx;k++)
		{
			RandomNum[i][j][k]=rand()%maxnum+1;
		}
	//读取随机路径
	if (ReadPath==1)
	{
		FILE *Pathfl;
		if ((err1 = fopen_s(&Pathfl, RPfilename.c_str(), "r")) != 0)
		{
			OutputErrorTipsMsg("===Random path file Path Error!!!");
			exit(0);
		}
		else
		{
			//Read file top
			fgets(top,99,Pathfl);
			fgets(top,99,Pathfl);
			fgets(top,99,Pathfl);
			fgets(top,99,Pathfl);
			fgets(top,99,Pathfl);
			fgets(top,99,Pathfl);
			//read conditional data
			for (i=0;i<Nz;i++)
				for(j=0;j<Ny;j++)
					for(k=0;k<Nx;k++)
					{
						fscanf_s(Pathfl,"%d\t%d\t%d\t%d",&v1,&v2,&v3,&v4);
						RR[v4].rpx=v1;RR[v4].rpy=v2;RR[v4].rpz=v3;
					}
		}
		fclose(Pathfl);
	}
	//为模拟网格进行排序并输出随机路径
	if (ReadPath==0)
	{
		//随机路径输出
		FILE *RRF; //随机路径输出文件指针
		err = fopen_s(&RRF, "RandomPath.txt", "w");
		fprintf(RRF, "RandomPath File\n");
		fprintf(RRF, "4\nx\ny\nz\norder\n");
		for(int m=0;m<maxnum;m++)
		{
			for (i=0;i<Nz;i++)
				for(j=0;j<Ny;j++)
					for(k=0;k<Nx;k++)
					{
						if(maxrandomnum<RandomNum[i][j][k])
						{
							maxrandomnum=RandomNum[i][j][k];
							RR[m].rpx=k;
							RR[m].rpy=j;
							RR[m].rpz=i;
						}
					}
					maxrandomnum=0;
					RandomNum[RR[m].rpz][RR[m].rpy][RR[m].rpx]=-1;     //防止已经作为路径的点干扰厚度路径
					//cout<<"random path point N.O."<<m+1<<":  "<<RR[m].rpx<<"   "<<RR[m].rpy<<endl;   //输出测试
					//输出随机路径
					fprintf(RRF,"%d\t%d\t%d\t%d\n",RR[m].rpx,RR[m].rpy,RR[m].rpz,m+1);
		}
		fclose(RRF);
	}
	GreenTipsMsg("Random Path is ready !!!");
}

//-----------------------------------------------------------------
//          Construct a geopattern with a original point
//
//  Input parameters: order （模拟顺序，随机路径的点号）
//  Original point from randompath array
//  注意：地质模式(GP)是一个全局变量，其长度为NCD。
//-----------------------------------------------------------------
void PatternConstruction(int order)
{
	int i=0,j=0,k=0,m=0;   //循环控制，m代表GP创建过程中获取的条件数据数量
	int	sx=0,sy=0,sz=0;  //搜索窗口大小
	int NN=0;  //搜索框内点数
	sx=searchwindowscale[iLevel][0];sy=searchwindowscale[iLevel][1];sz=searchwindowscale[iLevel][2];  //初始化搜索窗口大小,按照不同级次进行
	NCD=0;  //搜索窗内条件数据点的个数，初始化为0个；
	GPmatching=0;    //初始化不进行GP匹配
	//通过随机路径点创建地质模式
	if (TemplateType==0)
	{
	for(i=0;i<sz;i++)
		for (j=0;j<sy;j++)
			for (k=0;k<sx;k++)
			{ 
				if((RR[order].rpy+j-sy/2)>=0 && (RR[order].rpy+j-sy/2)<Ny && (RR[order].rpx+k-sx/2)>=0 && (RR[order].rpx+k-sx/2)<Nx && (RR[order].rpz+i-sz/2)<Nz && (RR[order].rpz+i-sz/2)>=0)
				{	
					if(DistanceType==0)
					{
						if(GridValue[RR[order].rpz+i-sz/2][RR[order].rpy+j-sy/2][RR[order].rpx+k-sx/2].discprop>-1)
							NCD++;
					}
					if(DistanceType==1)
					{
						if (GridValue[RR[order].rpz+i-sz/2][RR[order].rpy+j-sy/2][RR[order].rpx+k-sx/2].discprop>-1)
						//if (GridValue[RR[order].rpz + i - sz / 2][RR[order].rpy + j - sy / 2][RR[order].rpx + k - sx / 2].discprop>-1 && GridValue[RR[order].rpz][RR[order].rpy][RR[order].rpx].discprop<0)
						{
							NCD++;
						}
							
					}
					if(DistanceType==2)
					{
						if(GridValue[RR[order].rpz+i-sz/2][RR[order].rpy+j-sy/2][RR[order].rpx+k-sx/2].contprop>-1)
							NCD++;
					}
					NN++;
					//调试输出
					//cout<<"TIType="<<TIType<<"  搜索框内点数"<<NN<<"    NCD:"<<NCD<<"   DATA-"<<GridValue[RR[order].rpz+i-sz/2][RR[order].rpy+j-sy/2][RR[order].rpx+k-sx/2].contprop<<endl;

				}
			}
	}
	if (TemplateType == 1)
	{
		for (i = 0; i<NTemplateNode[iLevel]; i++)
		{
			if ((RR[order].rpy + TemplateNode[iLevel][i][1]) >= 0 && (RR[order].rpy + TemplateNode[iLevel][i][1])<Ny && (RR[order].rpx + TemplateNode[iLevel][i][0]) >= 0 && (RR[order].rpx + TemplateNode[iLevel][i][0])<Nx && (RR[order].rpz + TemplateNode[iLevel][i][2])<Nz && (RR[order].rpz + TemplateNode[iLevel][i][2]) >= 0)
			{
				if (DistanceType == 0)
				{
					if (GridValue[RR[order].rpz + TemplateNode[iLevel][i][2]][RR[order].rpy + TemplateNode[iLevel][i][1]][RR[order].rpx + TemplateNode[iLevel][i][0]].discprop > -1)
						NCD++;
				}
				if (DistanceType == 1)
				{
					if (GridValue[RR[order].rpz + TemplateNode[iLevel][i][2]][RR[order].rpy + TemplateNode[iLevel][i][1]][RR[order].rpx + TemplateNode[iLevel][i][0]].discprop > -1)
						//if (GridValue[RR[order].rpz + i - sz / 2][RR[order].rpy + j - sy / 2][RR[order].rpx + k - sx / 2].discprop>-1 && GridValue[RR[order].rpz][RR[order].rpy][RR[order].rpx].discprop<0)
					{
						NCD++;
					}

				}
				if (DistanceType == 2)
				{
					if (GridValue[RR[order].rpz + TemplateNode[iLevel][i][2]][RR[order].rpy + TemplateNode[iLevel][i][1]][RR[order].rpx + TemplateNode[iLevel][i][0]].contprop > -1)
						NCD++;
				}
				NN++;
			}
		}
	}
	//对条件数据点进行去掉处理
	if (GridValue[RR[order].rpz][RR[order].rpy][RR[order].rpx].discprop >-1)
		NCD = NCD - 1;

	//如果未获取到足够的条件数据点，则提示错误
	if (NCD<MinCDNum)
	{
		//记录NCD太小造成无法建立GP的网格点数
		NumInvalidGP[iTI][iLevel] = NumInvalidGP[iTI][iLevel] + 1;
		GPmatching=0;   //不进行GP创建和匹配
		//cout << "NCD<MinCDNum" << endl;
		if(DisplayDetails)
		{
			CString ERRTIP;
			ERRTIP.Format("NCD is less than MinCDNum: NCD=%d, MinCDNum=%d.  The current node is ignored.",NCD,MinCDNum);
			OutputErrorTipsMsg(ERRTIP);
			cout<<"\nLocation:\nrpx="<<RR[order].rpx<<"\trpy="<<RR[order].rpy<<"\trpz="<<RR[order].rpz<<"\nOrder="<<order<<"\n---------------"<<endl;
			//PlayTipsSound(1);
			if (NCD>=1)
			{
				PlayEmergencySound(1);
			}
		}
	}
	//如果搜索窗口内的条件数据点数量达标，则创建地质模式 GeoPattern	
	if(NCD>=MinCDNum)
	{
		/*if (GridValue[RR[order].rpz][RR[order].rpy][RR[order].rpx].discprop>-1)
		{
			NumCDGP++;
			cout << "Num CDGP="<<NumCDGP<<endl;
			Sleep(2000);
		}*/  //用于输出位于条件数据点的GP数量
		//首先记录数据点数
		//NDAVG[iLevel][iTI]=NDAVG[iLevel][iTI]+1.0*NCD/ConsideredCellNum;
		//动态分配地质模式数组并初始化
		GP=new GeoPattern[NCD];
		for(i=0;i<NCD;i++)
		{
			GP[i].facies=-1; GP[i].ContVariable=-1; GP[i].px=0; GP[i].py=0; GP[i].pz=0;
		}
		//创建GP
		//输出条件点提示	
		for (i=0;i<sz;i++)
			for (j=0;j<sy;j++)
				for (k=0;k<sx;k++)
				{
					if (!((j - sy / 2) == 0 && (k - sx / 2) == 0 && (i - sz / 2)==0))
					if((RR[order].rpy+j-sy/2)>=0 && (RR[order].rpy+j-sy/2)<Ny && (RR[order].rpx+k-sx/2)>=0 && (RR[order].rpx+k-sx/2)<Nx && (RR[order].rpz+i-sz/2)<Nz && (RR[order].rpz+i-sz/2)>=0)
					{
						if(DistanceType==0)
							if(GridValue[RR[order].rpz+i-sz/2][RR[order].rpy+j-sy/2][RR[order].rpx+k-sx/2].discprop>-1)
								{
									//针对不同类型的变量给定GP内部各点的属性
									GP[m].facies=GridValue[RR[order].rpz+i-sz/2][RR[order].rpy+j-sy/2][RR[order].rpx+k-sx/2].discprop;
									//计算GP内部各点的相对坐标
									GP[m].px=k-sx/2;GP[m].py=j-sy/2;GP[m].pz=i-sz/2;
									//计数器
									m++;
									//调试输出
									//cout<<"PatternConstruction: 数量"<<m-1<<"坐标"<<GP[m-1].px<<"  "<<GP[m-1].py<<"   "<<GP[m-1].pz<<endl;
									//Sleep(1000);
								}
						if(DistanceType==1)
							if(GridValue[RR[order].rpz+i-sz/2][RR[order].rpy+j-sy/2][RR[order].rpx+k-sx/2].discprop>-1)
								{
									//针对不同类型的变量给定GP内部各点的属性
									GP[m].facies=GridValue[RR[order].rpz+i-sz/2][RR[order].rpy+j-sy/2][RR[order].rpx+k-sx/2].discprop;
									//计算GP内部各点的相对坐标
									GP[m].px=k-sx/2; GP[m].py=j-sy/2; GP[m].pz=i-sz/2;
									//计数器
									m++;
								}
						if(DistanceType==2)
							if(GridValue[RR[order].rpz+i-sz/2][RR[order].rpy+j-sy/2][RR[order].rpx+k-sx/2].contprop>-1)
								{
									//针对不同类型的变量给定GP内部各点的属性
									GP[m].ContVariable=GridValue[RR[order].rpz+i-sz/2][RR[order].rpy+j-sy/2][RR[order].rpx+k-sx/2].contprop;
									//计算GP内部各点的相对坐标
									GP[m].px=k-sx/2;GP[m].py=j-sy/2;GP[m].pz=i-sz/2;
									//计数器
									m++;
									//调试输出
									//cout<<"PatternConstruction: 数量"<<m-1<<"坐标"<<GP[m-1].px<<"  "<<GP[m-1].py<<"   "<<GP[m-1].pz<<endl;
									//Sleep(500);
								}
					}
				}
		GPmatching=1;
	}
	//if(DisplayDetails==1)
	//cout<<"---NCD:"<<NCD<<"     M:"<<m<<endl;   //NCD是条件数据个数，m是GP中实际获取到的条件数据。
}

//-----------------------------------------------------------------
// 在训练图像中搜寻Patterns，并与Conditional Ppatterns进行匹配对比
// 从而求取数据事件距离的最小值
//-----------------------------------------------------------------
void search_match(int order) 
{
	int Nvisit=0;  //当前order条件下，GP是否有进行有效的计算
	int NenoughnodesinTIGP = 0;  //当前order条件下，GP是否有进行有效的计算
	//Variables to control the loop
	int CDorder=0;
	double md=0.0;  //Matching Degree
	int ND=0;  //落在训练图像内的条件点的个数；
	double MatchingNum=0;   //搜索过程中Pattern 与 GP的吻合程度，其计算标准复杂，应当综合考虑点数、吻合点数等
	CString TIPs;
	//double MaxMatch=0.0;   //每个数据事件对比中最大的匹配度
	double minDistance=1000;  //最小距离
	int naz=0;  //排除非无效值后参与计算的点数
	CString  warntip;    //错误、警告提示
	int displayGP=0;   //在匹配度较低或获取不到数据事件的条件下显示GP的开关
	double DTop=0,DBase=0;   //数据事件距离计算中，公式的分子分母
	double ED=0;   //用于计算当前点与中心点的欧式距离
	double mddisp = 0; //用于调试输出，查看naz=0时的md
	int tix = 0, tiy = 0, tiz = 0; //用于调试输出，查看TI GP
	match=0;
	//First: Search for the similar GeoPattern within the TI
	//How to search and find the most similar patterns?
	//按照训练图像进行顺序扫描
	for (int i=0;i<TINz;i++)
		for (int j=0;j<TINy;j++)
			for (int k=0;k<TINx;k++)
				{
					naz = 0;
					//minDistance=1000;
					//匹配-相等则为1，不等则不加--后续需要改进。
					for (CDorder=0;CDorder<NCD;CDorder++)   
					{
						tiz = i; tiy = j; tix = k;  //用于调试输出，查看TI GP
						int NED_0 = 0;
						if((k+GP[CDorder].px)<TINx && (k+GP[CDorder].px)>=0 && (j+GP[CDorder].py)<TINy && (j+GP[CDorder].py)>=0 && (i+GP[CDorder].pz)<TINz && (i+GP[CDorder].pz)>=0)
						{
							//Distance type 0: The simple type
							if (DistanceType==0)
							{
								ND++;
								if (GP[CDorder].facies!=TIValue[i+GP[CDorder].pz][j+GP[CDorder].py][k+GP[CDorder].px] && TIValue[i+GP[CDorder].pz][j+GP[CDorder].py][k+GP[CDorder].px]>-1)    //改为计算数据事件距离   2015-05-28
								{
									MatchingNum+=1;    //这里采用距离类型0-计算最简单的距离，距离的最大值为1，最小值为0.
								}
							}
							//Distance type 1:Weighting based on distance
							if (DistanceType==1)
							{
								ND++;
								ED=sqrt(1.0*(GP[CDorder].px*GP[CDorder].px+GP[CDorder].py*GP[CDorder].py+GP[CDorder].pz*GP[CDorder].pz));  //当前点的欧氏距离
								if (GP[CDorder].facies!=TIValue[i+GP[CDorder].pz][j+GP[CDorder].py][k+GP[CDorder].px] && ED>0 && TIValue[i+GP[CDorder].pz][j+GP[CDorder].py][k+GP[CDorder].px]>-1)     // && TIValue[i+GP[CDorder].pz][j+GP[CDorder].py][k+GP[CDorder].px]>-99
									{
										//cout << TIValue[i + GP[CDorder].pz][j + GP[CDorder].py][k + GP[CDorder].px] << "    " << i + GP[CDorder].pz << "   " << j + GP[CDorder].py << "   " << k + GP[CDorder].px << endl;
										//cout << i << "   " << j << "   " << k << endl;
										//cout << GP[CDorder].pz << "   " << GP[CDorder].py << "   " << GP[CDorder].px << endl;
										//cout << "-------------------------------------------" << endl;
										//cout << "gp facies: " << GP[CDorder].facies << "    tivalue:" << TIValue[i + GP[CDorder].pz][j + GP[CDorder].py][k + GP[CDorder].px] << endl;
										//if (TIValue[i + GP[CDorder].pz][j + GP[CDorder].py][k + GP[CDorder].px]!=1)
										//{
										//	Sleep(10000);
										//}
										DTop=DTop+1.0/ED;
										naz++;
										//cout << "NCD= " << NCD<<"   "<<"ND= "<<ND<<"   "<<"DTop="<<DTop<<"\n"<<endl;
										//Sleep(50);
									}
								if (GP[CDorder].facies == TIValue[i + GP[CDorder].pz][j + GP[CDorder].py][k + GP[CDorder].px] && ED>0 && TIValue[i + GP[CDorder].pz][j + GP[CDorder].py][k + GP[CDorder].px]>-1)
								{
									naz++;
								}
							}
							//Distance type 2:Distance of continual variables
							//计算距离的函数在本次修改中未更改，貌似是对的-2015-05-28
							if (DistanceType==2)
							{
								ND++;
								ED=sqrt(1.0*(GP[CDorder].px*GP[CDorder].px+GP[CDorder].py*GP[CDorder].py+GP[CDorder].pz*GP[CDorder].pz));  //数据事件距离
								if (ED == 0)
								{
									NED_0 = NED_0 + 1;
									nED_0total = nED_0total + 1;
								}
								if (ED>0)
								{
									if (ContTIValue[i+GP[CDorder].pz][j+GP[CDorder].py][k+GP[CDorder].px]>-99)
									{
										DTop=DTop+1.0*(sqrt(1.0*(GP[CDorder].ContVariable-ContTIValue[i+GP[CDorder].pz][j+GP[CDorder].py][k+GP[CDorder].px])*(GP[CDorder].ContVariable-ContTIValue[i+GP[CDorder].pz][j+GP[CDorder].py][k+GP[CDorder].px])))/ED;
										naz++;
									}
								}
								//cout<<"ED="<<ED<<"   dtop="<<DTop<<endl;
								//Sleep(1000);	
							}
						}   

						if (NED_0>1)
						{
							cout << "N ED_0: " << NED_0 <<"     nED_0total: "<< nED_0total <<"    x,y,z:"<<RR[order].rpx<<","<< RR[order].rpy<<","<< RR[order].rpz << endl;
						}
						NED_0 = 0;
					}  //针对一个GP的计算完毕
				//	if (GridValue[RR[order].rpz][RR[order].rpy][RR[order].rpx].discprop > -1)
				//		cout << "完成对条件点的计算" <<"ND: "<< ND<<"MinCDNum: " << MinCDNum <<"dtop="<<DTop<< endl;

					//对计算的结果进行判断
					if (ND >= MinCDNum && ND>0)   //MinD 为输入参数，指定在一个模式中，最少的条件数据量,还需要做一些处理完善意外情况
					{
						Nvisit = Nvisit + 1;  //记录目前order是否有达到过要求最小的训练图像网格点数;
						if(DistanceType==0) md=MatchingNum/ND;
						if(DistanceType==1) md=DTop/ND;
						if(DistanceType==2) md=DTop/ND;
					//	if (GridValue[RR[order].rpz][RR[order].rpy][RR[order].rpx].discprop > -1)
					//	{
					//		cout << "1--md=" << md << "   rpx:" << RR[order].rpx << "   rpy:" << RR[order].rpy << "   rpz:" << RR[order].rpz << endl;
					//		cout << "naz:" << naz << "   NCD*MinD:" << NCD*MinD<< endl;
					//		Sleep(5000);
					//	}
						if (naz<int(NCD*MinD))
						{
							NenoughnodesinTIGP = NenoughnodesinTIGP + 1;
							md = 1000;
							naz = 0;
						}
						//求取最小的数据事件距离
						if(md<minDistance)
						{
							//给MatchingDegree数组赋值
							MatchingDegree[iTI][iLevel][RR[order].rpz][RR[order].rpy][RR[order].rpx]=md;
							//cout<<"MD="<<md<<endl;
					//		if (GridValue[RR[order].rpz][RR[order].rpy][RR[order].rpx].discprop > -1)
					//		{
					//			cout << "2--md=" << md << endl;
					//			cout << "!!!!!!!!!!!!!" << endl;
					//			Sleep(5000);
					//		}
							//赋值给minDistance
							minDistance=md;
							mddisp = md;
							//输出测试信息
							//cout << "ND:" << ND << "   MD:" << md << "   NAZ:" << naz << endl;
							if (md<=MinMatchingDegree)  goto stend2;
						}
					}
					//执行完数据事件距离计算之后，对重要参数进行归零
					ND=0;   //落在训练图像内的点数（应对训练图像搜索至边缘部位的情况）
					MatchingNum=0.0;    //针对数据事件距离计算方法0，即离散变量的未加权计算。
					md=0;    //最小距离的归零，没有实际作用。
					DTop=0;    //数据事件计算公式1对应的函数分子归零，保护计算结果。
					DBase=0;    //数据事件计算公式1对应的函数分母归零，保护计算结果。
				}
			    //判断这个order是否有进行有效的计算，这里已经排除了无法创建GP的情况
				if (Nvisit <=0)
					NumNotEnoughNodesinTI[iTI][iLevel] = NumNotEnoughNodesinTI[iTI][iLevel] + 1;
				if (NenoughnodesinTIGP <= 0)
					NumNotEnoughNodesinTIGP[iTI][iLevel] = NumNotEnoughNodesinTIGP[iTI][iLevel] + 1;
			//对匹配度未能达到1.0，即未能完全匹配的（一般是离散型变量）进行详细信息显示
			if(MatchingDegree[iTI][iLevel][RR[order].rpz][RR[order].rpy][RR[order].rpx]>0.00001)    //对于未达到距离为0的情况进行详细信息的显示
			{
				if(DisplayException==1)  //如果警告提示开启
				{
					warntip.Format("The matching Degree finally is %f ,order:%d,NCD=%d",MatchingDegree[iTI][iLevel][RR[order].rpz][RR[order].rpy][RR[order].rpx],order,NCD);
					YellowWarnMsg(warntip);
					cout<<"Location:\nx="<<RR[order].rpx<<"\ty="<<RR[order].rpy<<"\tz="<<RR[order].rpz<<endl;
					cout<<"Display GP? Yes-1; NO-0"<<endl;
					cin>>displayGP;
					if (displayGP==1)
					{
						cout<<"--------------- GP Body Title ---------------"<<endl;
						for (int i=0;i<NCD;i++)
						{
							cout<<"Facies:"<<GP[i].facies<<"  px:"<<GP[i].px<<"  py:"<<GP[i].py<<"  pz:"<<GP[i].pz<<endl;
						}
						cout<<"---------------- GP Body End ----------------"<<endl;
					}
					system("pause");
				}
				goto stend2;
			}
		stend2: ;
			//输出条件GP和训练GP
			if (DisplaySearch_Match==1)
			{
				cout << "--------------------------------Top--------------------------------" << endl;
				cout << "Search Order:" << (tix + 1)*(tiy + 1)*(tiz + 1) << endl;
				cout << "Cond GP" << endl;
				for (int a = 0; a < NCD; a++)
				{
					cout << GP[a].facies << "\t";
				}
				cout << endl;
				cout << "TI GP" << endl;
				for (CDorder = 0; CDorder < NCD; CDorder++)
					if ((tix + GP[CDorder].px) < TINx && (tix + GP[CDorder].px) >= 0 && (tiy + GP[CDorder].py) < TINy && (tiy + GP[CDorder].py) >= 0 && (tiz + GP[CDorder].pz) < TINz && (tiz + GP[CDorder].pz) >= 0)
					{
						cout << TIValue[tiz + GP[CDorder].pz][tiy + GP[CDorder].py][tix + GP[CDorder].px] << "\t";
					}
					else
						cout << "X\t";
				cout << endl;
				cout << "================================Bot================================" << endl;
				Sleep(2000);
				if (mddisp > 0.00000001 && naz > 0)
				{
					cout << "naz=" << naz << "   md=" << mddisp << endl;
					PlayTipsSound(1);
				}
			}
			//cout << "MD=" << MatchingDegree[iTI][iLevel][RR[order].rpz][RR[order].rpy][RR[order].rpx]<<endl;   //测试输出
}


//-----------------------------------------------------------------
//         GeoPaterns Reproduction with the GP get before
//
//  Data Used:GP,NCD
//  
//-----------------------------------------------------------------
void GeoPatternMatching(int	order)
{
	//首先搜寻与GP最相似的Pattern
	search_match(order);
	//if(DisplayDetails==1)
	//cout<<"NCD:"<<NCD<<"\torder:"<<order<<endl;    //输出匹配的数据事件条件数据个数及当前位置
	//进行模拟--暂时不做模拟，因此注释掉，备后续使用
}


//-----------------------------------------------------------------
//                   Matching Results output
//
//  Data Used:MatchingDegree,NCD
//  Format:1--GSlib & Petrel
//  
//-----------------------------------------------------------------
void OutputMatchingResults(long **timecost)
{
	int i=0,j=0,k=0,m=0,n=0;
	//输出文件初始化
	ofstream fout;  //输出结果文件
	ofstream Exceptionout;  //输出意外网格点统计数据数据
	fout.open(MatchingResultfl);
	//File definitions
	fout<<"         Training Image Selection(TIS) Result Version 2016-06-17 "<<"CLOCKS_PER_SEC="<<CLOCKS_PER_SEC<<"\n";
	fout<<NLevel*NTI+1<<"\n";
	if(TIType==0)   fout<<"Facies CondData\n";
	if(TIType==1)   fout<<"Continual CondData\n";
//	if(TIType==0)   fout << "Last TI\n";   //输出最后一幅训练图像
	for(j=0;j<NTI;j++)
		for(int i=0;i<NLevel;i++)
			fout<<"General_TI_"<<TrainingImageFl[j]<<"_"<<searchwindowscale[i][0]<<"*"<<searchwindowscale[i][1]<<"*"<<searchwindowscale[i][2]<<"_timecost="<<timecost[j][i]<<"ms"<<"\n";
			//fout<<"MatchingDegree_TI"<<TrainingImageFl[j]<<" Search_window_scale(x,y,z):"<<searchwindowscale[i][0]<<","<<searchwindowscale[i][1]<<","<<searchwindowscale[i][2]<<"--timecost="<<timecost[j][i]<<"ms"<<"\n";
			//fout<<"MatchingDegree_TI"<<j+1<<"_Level"<<i+1<<"\n";
	//Reals
	for (i = Nz-1; i>=0; i--)
		for(j=0;j<Ny;j++)
			for(k=0;k<Nx;k++)
			{
				if(TIType==0) fout<<GridValue[i][j][k].discprop<<"\t";
				if(TIType==1) fout<<GridValue[i][j][k].contprop<<"\t";
				//if (TIType == 0) fout << TIValue[i][j][k] << "\t";   //输出训练图像做测试
				for(m=0;m<NTI;m++)
					for(n=0;n<NLevel;n++)
					{
						fout<<MatchingDegree[m][n][i][j][k]<<"\t";
					}
				fout<<"\n";
			}
			fout.close();
			GreenTipsMsg("Matching result output completed!!!");
			GreenTipsMsg(MatchingResultfl);
	//输出意外网格点统计数据
	Exceptionout.open("Exceptionout.txt");
	Exceptionout << "意外数据统计结果\n";
	for (i = 0; i < NTI; i++)
	{
		Exceptionout << "TI-" << i << "\n";
		for (j = 0; j < 3;j++)
		{ 
			if (j == 0) Exceptionout << "NumInvalidGP\t";
			if (j == 1) Exceptionout << "NumNotEnoughNodesinTI\t";
			if (j == 2) Exceptionout << "NumNotEnoughNodesinTIGP\t";
			for (k = 0; k < NLevel;k++)
			{
				if (j == 0) Exceptionout << NumInvalidGP[i][k]<<"\t";
				if (j == 1) Exceptionout << NumNotEnoughNodesinTI[i][k]<<"\t";
				if (j == 2) Exceptionout << NumNotEnoughNodesinTIGP[i][k]<<"\t";
			}
			Exceptionout << "\n";
		}
		Exceptionout << "\n";
	}
	Exceptionout.close();
}

void OutputAverageNumberofDataCovered()
{
	int i=0,j=0;
	//输出文件初始化
	ofstream fout;
	fout.open("CDout.dat");
	//File definitions
	fout<<"         Training Image Selection(TIS) Result-Average of Number of Data Covered in Data Events"<<"\n";
	fout<<NLevel*NTI<<"\n";
	for(j=0;j<NTI;j++)
		for(int i=0;i<NLevel;i++)
			fout<<"TI"<<TrainingImageFl[j]<<"SWS_"<<searchwindowscale[i][0]<<"*"<<searchwindowscale[i][1]<<"*"<<searchwindowscale[i][2]<<"\n";
			//fout<<"TI"<<TrainingImageFl[j]<<"SWS_"<<searchwindowscale[i][0]<<"*"<<searchwindowscale[i][1]<<"*"<<searchwindowscale[i][2]<<"_CDAVG="<<NDAVG[i][j]<<"\n";
	fout.close();
	GreenTipsMsg("NDAVG output completed!!!");
}



//-----------------------------------------------------------------
//                     Main Function
//-----------------------------------------------------------------
void main()
{
	//参数准备
	int CellNum=0;   //平面上的网格点数
	clock_t start,finish;  //获取运算起始时间 
	long **timecost;   //运行时间
	struct tm *ptr;   //时间结构体
	int PRatio=0;  //计算比例
	string RPfilename = "                               ";	 //读取的随机路径文件名
	time_t lt; 
	lt =time(NULL); 
	//Version info
	WriteVersion();	

	//Read Parameter,Conditional data and TI
	if(ReadParaAndData()==0)
		goto endline;
	else
	{
		//初始化运行时间数组
		timecost=new long*[NTI];
		for(int i=0;i<NTI;i++)
			timecost[i]=new long[NLevel];
		//generate a set of random numbers with the seed
		srand(seed);
		//Set a random path.
		//是否读取现成的随机路径
		cout<<"Read Random Path? 0-No, 1-Yes"<<endl;
		cin>>ReadPath;
		if (ReadPath == 1)
		{
			cout << "Input the Random Path file name:" << endl;
			cin >> RPfilename;
		}
		RandomPathConstruction(RPfilename);
		//初始化参数
		CellNum=Nx*Ny*Nz;
		ConsideredCellNum=int(CellNum*CalRatio);
		//针对不同的训练图像进行分析
		for (iTI=0;iTI<NTI;iTI++)
		{
			TINx=TIscalex[iTI]; TINy=TIscaley[iTI]; TINz=TIscalez[iTI]; 
			//离散型训练图像数组初始化
			if(TIType==0)
			{
				if(iTI>0)
					DeAllocateTrainingImageValue(TIscalex[iTI-1],TIscaley[iTI-1],TIscalez[iTI-1]);  //释放数组
				AllocateTrainingImageValue();     //动态分配训练图像数组
			}
			//连续型训练图像数组初始化
			if(TIType==1)
			{
				if(iTI>0)
					DeAllocateTrainingImageValue(TIscalex[iTI-1],TIscaley[iTI-1],TIscalez[iTI-1]);  //释放数组
				AllocateContTrainingImageValue();
			}	
			//训练图像读取
			if(readTI(TrainingImageFl[iTI])==0)         //读取TI
			{
				OutputErrorTipsMsg("===check your Training Image file path!!!");
				exit(0);
			}
			
			//--------------------Perform Analysis---------------------
			//Stage 1：
			//对不同的搜索窗进行计算
			for(iLevel=0;iLevel<NLevel;iLevel++)
			{
				NumCDGP = 0;   //记录条件点是否有创建GP，这里归零。
				//记录起始时间
				start=clock();
				//显示当前工作的尺度level
				CString levelnow;
				levelnow.Format("Working on TI:%d   level: %d",iTI+1,iLevel+1);
				GreenTipsMsg(levelnow);
				Sleep(1000);
				//Set a random path.
				//RandomPathConstruction();
				//暂时不启用循环
				for(int i=0;i<ConsideredCellNum;i++)     //如何循环还需要定义和进一步设计，应当使用随机路径，需要重新设计排序方法。---已启用
					{
						//step 1:Setup the search patterns
						//if(DisplayDetails==1)
						//cout<<"****************************************************************"<<endl;
						PatternConstruction(i);
						//step 2:Patterns matching
						if(GPmatching==1)
							GeoPatternMatching(i);
						if(((i+1)%(int(ConsideredCellNum/10)))==0)
						{
							lt =time(NULL); 
							ptr=gmtime(&lt); 
							printf(ctime(&lt)); 
							PRatio=int(floor(100.0*(i+1)/ConsideredCellNum+0.5));
							cout<<"Progressing: "<<"TI->"<<TrainingImageFl[iTI]<<"  Search Window scale->"<<"x-"<<searchwindowscale[iLevel][0]<<"  y-"<<searchwindowscale[iLevel][1]<<"  z-"<<searchwindowscale[iLevel][2]<<"----"<<PRatio<<"%"<<endl;
						}
					}
				finish=clock();
				timecost[iTI][iLevel]=finish-start;
				//NDAVG[iLevel][iTI]=1.0*ND[iLevel][iTI]/ConsideredCellNum;
				//end loop
			}
		}
		OutputMatchingResults(timecost);
		//OutputAverageNumberofDataCovered();
	}
	//End line to exit.
endline: Sleep(100);
	system("pause");
}