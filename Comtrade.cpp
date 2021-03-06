#include "StdAfx.h"
#include "Comtrade.h"
#include "calc.h"
#include <vector>
#include <complex>
#include "splab_vector.h"
#include "ComTime.h"

#define TEMP_LENGTH 255


CComtrade::CComtrade(void)
{
}
CComtrade::CComtrade(const CString &FileName)
: m_Line1(_T(""))
, m_Line2(_T(""))
, m_TotalNumber(0)
, m_ANumber(0)
, m_DNumber(0)
, m_VNumber(0)
, m_LF(0.0)
, m_NRates(0)
, m_TotalSampleNum(0)
, m_TimeMult(0.0)
{
	int curPos = 0;

	maxVoltageData=0;
	maxCurrentData=0;
	maxFrequencyData=0;

	//绘图尺寸设定
	m_drawParam.defaultVSpace = 100;
	m_drawParam.defaultDVSpace = 40;
	m_drawParam.VGap = 20;
	m_drawParam.DGap = 10;
	m_drawParam.topSpace = 60;
	m_drawParam.rightSpace = 150;
	m_drawParam.leftSpace = 50;
	m_drawParam.defaultSPScale = 1;
	m_drawParam.SPScale = 1;
	m_drawParam.LastSPScale = 1;
	m_drawParam.defaultDPScale = 1.0/20;

	CString temp;

	pCfgFile.Open(FileName+_T(".cfg"), CFile::modeRead | CFile::typeText, NULL);
	pDatFile.Open(FileName+_T(".dat"), CFile::modeRead | CFile::typeBinary, NULL);

	//读取CFG文件
	m_Line1 = pCfgFile.ReadLine();
	m_Line2 = pCfgFile.ReadLine();
	m_TotalNumber = _wtoi((m_Line2.Tokenize(_T(",AD"), curPos).GetBuffer()));
	m_ANumber = _wtoi((m_Line2.Tokenize(_T(",AD"), curPos).GetBuffer()));
	m_DNumber = _wtoi((m_Line2.Tokenize(_T(",AD"), curPos).GetBuffer()));

	AVector.resize(m_ANumber);
	DVector.resize(m_DNumber);

	CString Temp_Line;
	int _An,_Dn,_y;
	CString _name,_ph,_ccbm,_uu; 
	double _multiplier,_offset,_skew;
	int _min, _max;   
	double _primary,_secondary;
	CString _PS;
	for(int i =0;i!=m_ANumber;i++)
	{
		curPos=0;
		Temp_Line = pCfgFile.ReadLine();
		_An = _wtoi((Temp_Line.Tokenize(_T(","), curPos).GetBuffer()));
		_name = Temp_Line.Tokenize(_T(","), curPos).GetBuffer();
		_ph = Temp_Line.Tokenize(_T(","), curPos).GetBuffer();
		_ccbm  = Temp_Line.Tokenize(_T(","), curPos).GetBuffer();
		_uu = Temp_Line.Tokenize(_T(","), curPos).GetBuffer();
		_multiplier =  _wtof((Temp_Line.Tokenize(_T(","), curPos).GetBuffer()));
		_offset = _wtof((Temp_Line.Tokenize(_T(","), curPos).GetBuffer()));
		_skew = _wtof((Temp_Line.Tokenize(_T(","), curPos).GetBuffer()));
		_min = _wtoi((Temp_Line.Tokenize(_T(","), curPos).GetBuffer()));
		_max = _wtoi((Temp_Line.Tokenize(_T(","), curPos).GetBuffer()));
		_primary = _wtof((Temp_Line.Tokenize(_T(","), curPos).GetBuffer()));
		_secondary = _wtof((Temp_Line.Tokenize(_T(","), curPos).GetBuffer()));
		_PS = Temp_Line.Tokenize(_T(","), curPos).GetBuffer();
		AVector[i] = new AChannel(_An,_name, _ph,_ccbm,_uu,_multiplier,_offset,_skew,_min,_max,_primary,_secondary,_PS,1);
		//读取并设定绘图参数
		AVector[i]->m_drawPara.defultVSpace = m_drawParam.defaultVSpace;
		AVector[i]->m_drawPara.VSpace = m_drawParam.defaultVSpace;
	}
	for(int i =0;i!=m_DNumber;i++)
	{
		curPos=0;
		Temp_Line = pCfgFile.ReadLine();
		_Dn = _wtoi((Temp_Line.Tokenize(_T(","), curPos).GetBuffer()));
		_name = Temp_Line.Tokenize(_T(","), curPos).GetBuffer();
		_ph = Temp_Line.Tokenize(_T(","), curPos).GetBuffer();
		_ccbm  = Temp_Line.Tokenize(_T(","), curPos).GetBuffer();
		_y = _wtoi((Temp_Line.Tokenize(_T(","), curPos).GetBuffer()));
		DVector[i]= new DChannel(_Dn,_name,_ph,_ccbm,_y);
	}
	Temp_Line = pCfgFile.ReadLine();
	m_LF = _wtof(Temp_Line.GetBuffer());
	Temp_Line = pCfgFile.ReadLine();
	m_NRates = _wtoi(Temp_Line.GetBuffer());
	m_SampleRate.resize(m_NRates);
	for(int i =0;i!=m_SampleRate.size();i++)
	{
		curPos=0;
	    Temp_Line = pCfgFile.ReadLine();
		m_SampleRate[i].samp = _wtof((Temp_Line.Tokenize(_T(","),curPos).GetBuffer()));
		m_SampleRate[i].endsamp = _wtoi((Temp_Line.Tokenize(_T(","),curPos).GetBuffer()));
	}
	m_TotalSampleNum = m_SampleRate.at(m_SampleRate.size()-1).endsamp;
	
	Sample.resize(m_TotalSampleNum);                 //初始化采样号和时间戳向量存储空间
	Time.resize(m_TotalSampleNum);

	for(int i =0;i!=m_ANumber;i++)               //开始初始化data向量存储空间
		AVector.at(i)->AData.resize(m_TotalSampleNum);
	for(int i =0;i!=m_DNumber;i++)
		DVector.at(i)->DData.resize(m_TotalSampleNum);
	CComTime time1(pCfgFile.ReadLine());
	CComTime time2(pCfgFile.ReadLine());
	m_StartTime = time1;
	m_EndTime = time2;
	m_DataFileType = pCfgFile.ReadLine();
	Temp_Line = pCfgFile.ReadLine();
	m_TimeMult = _wtof(Temp_Line.GetBuffer());

	//CFG文件读取完毕
	//AVector/DVector/AData/DData has been resized.

	//读取DAT文件
	std::vector<short int> Analog;  //模拟数据暂存
	std::vector<BOOL> Status;       //数字数据暂存
	int m_tDNumber;                 //数字通道字节数
	short int OriStatus;            //状态原始数据
//	unsigned int Sample;            //记录号
//	int Time;                       //时间戳
	short int temp_data;     //原始数据
	double temp_value;       //真实数据

	if(((float)m_DNumber)/16==m_DNumber/16)
	{
		m_tDNumber = m_DNumber/16;
	}
	else
	{
		m_tDNumber = m_DNumber/16 + 1;
	}

	//pDatFile.Read((char*)&Sample, sizeof(int));
	//pDatFile.Read((char*)&Time, sizeof(int));
	//Analog.resize(m_ANumber);
	//Status.resize(m_DNumber);
	for(int j = 0; j != m_TotalSampleNum; ++j)
	{
		pDatFile.Read((char*)&(Sample[j]), sizeof(int));
		pDatFile.Read((char*)&(Time[j]), sizeof(int));
		for(int i = 0; i != m_ANumber; ++i)
		{
			pDatFile.Read(&temp_data, sizeof(short int));
			temp_value=temp_data*AVector[i]->multiplier+AVector[i]->offset;
			AVector[i]->AData[j]=temp_value;
			temp_value=abs(temp_value);
			if (temp_value > AVector[i]->maxAData)
			{
				AVector[i]->maxAData = temp_value;
			}
		}
		for(int i = 0; i!=m_tDNumber; ++i)
		{
			pDatFile.Read((short int*)&OriStatus, sizeof(short int));
			for(int ii = 0; ii!=16; ++ii)
			{
				//Status[i] = OriStatus & (1UL<<i);
				DVector[i*16+ii]->DData[j] = (OriStatus & (1UL<<ii))>>ii;
			}
		}
	}
	for (int i = 0; i != m_ANumber; ++i)
	{
		if(AVector[i]->uu==_T("V"))
		{
			if (AVector[i]->maxAData>maxVoltageData)
			maxVoltageData=AVector[i]->maxAData;
		}
		else if(AVector[i]->uu==_T("A"))
		{
			if (AVector[i]->maxAData>maxCurrentData)
			maxCurrentData=AVector[i]->maxAData;
		}
		else if(AVector[i]->uu==_T("Hz"))
		{
			if (AVector[i]->maxAData>maxFrequencyData)
			maxFrequencyData=AVector[i]->maxAData;	
		}
	}
	m_drawParam.defaultVPScale = maxVoltageData/(m_drawParam.defaultVSpace/2);
	m_drawParam.defaultAPScale = maxCurrentData/(m_drawParam.defaultVSpace/2);
	m_drawParam.defaultHPScale = maxFrequencyData/(m_drawParam.defaultVSpace/2);
	for (int i = 0; i != m_ANumber; ++i)
	{
		AVector[i]->m_drawPara.VSpace=m_drawParam.defaultVSpace;
		switch(AVector[i]->uu[0])
		{
		case _T('V'):
			{
				AVector[i]->m_drawPara.UPScale=m_drawParam.defaultVPScale;
				break;
			}
		case _T('A'):
			{
				AVector[i]->m_drawPara.UPScale=m_drawParam.defaultAPScale;
				break;
			}
		case _T('H'):
			{
				AVector[i]->m_drawPara.UPScale=m_drawParam.defaultHPScale;
				break;
			}
		}
		
		AVector[i]->m_drawPara.defaultUPScale=AVector[i]->m_drawPara.UPScale;
	}
	pCfgFile.Close();
	pDatFile.Close();
}
CComtrade::~CComtrade(void)
{
	for(int i=0; i<this->m_ANumber;i++)
		delete this->AVector[i];
	for(int i=0; i<this->m_DNumber;i++)
		delete this->DVector[i];

}

CDatFile::CDatFile(void)
{
}

CDatFile::~CDatFile(void)
{
}

CCfgFile::CCfgFile(void)
{
}

CCfgFile::~CCfgFile(void)
{
}

AChannel CComtrade::Evaluator(const CString& inputExp)
{
	return EVA(inputExp,this);
}

CString CCfgFile::ReadLine(void)   //从ANSI文件中读取一行CString
{
	CString Temp_Line;
	char x,_x='\0';
	char y[TEMP_LENGTH]={'\0'};
	for(int i=0;i<TEMP_LENGTH;i++)
	{
		this->Read(&x,1);
		if (x=='\n')
			break;
		if((x==',')&&(_x==','))
			y[i++]=' ';
		y[i]=x;
		_x=x;
	}
	Temp_Line=y;
	return Temp_Line;
}

int CComtrade::FormAChannelGroup(void)
{
	return 0;
}


int CComtrade::GetCurrentSeqComp(int CurrentPoint, int ch_A, int ch_B, int ch_C, int Index)
{
	using splab::PI;
	using std::complex;
	using std::polar;
	ch_A--;
	ch_B--;
	ch_C--;
	int Flag=1;
	Flag = this->AVector[ch_A]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	if(Flag==1)
	{
		this->AVector[ch_A]->APhasor.Amplitude =0.0;
		this->AVector[ch_A]->APhasor.phase = 0.0;
		return 1;
	}
	this->AVector[ch_B]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	this->AVector[ch_C]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	complex<double> A,B,C;
	complex<double> alpha(-0.5,std::sqrt(3.0)/2),alpha2(-0.5,-std::sqrt(3.0)/2);
	A =polar(AVector[ch_A]->APhasor.Amplitude,AVector[ch_A]->APhasor.phase*PI/180);
	B =polar(AVector[ch_B]->APhasor.Amplitude,AVector[ch_B]->APhasor.phase*PI/180);
	C =polar(AVector[ch_C]->APhasor.Amplitude,AVector[ch_C]->APhasor.phase*PI/180);
	this->CurentSeqComp[Index-1].PSC = (A + alpha * B + alpha2 * C)*complex<double>(1/3.0,0);   //最大值
	this->CurentSeqComp[Index-1].NSC = (A + alpha2 * B + alpha * C)*complex<double>(1/3.0,0);   //最大值
	this->CurentSeqComp[Index-1].ZSC = (A + B + C)*complex<double>(1/3.0,0);
	this->CurentSeqComp[Index-1].name = this->AVector[ch_A]->ccbm;
	return 0;
}

int CComtrade::GetCurrentZ(int CurrentPoint, int ch_UA, int ch_UB, int ch_UC, int ch_IA, int ch_IB, int ch_IC , int Type, LineData TempData)
{
	using std::complex;
	using std::polar;
	using splab::PI;
	//type=1 接地阻抗
	//type=2 单相阻抗
	//type=3 相间阻抗
	int Flag = 1;
	Flag = this->AVector[--ch_UA]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	if (Flag == 1)
	{
		z.R1 = 0;z.X1 = 0;
		z.R2 = 0;z.X2 = 0;
		z.R3 = 0;z.X3 = 0;
		return 1;
	}
	this->AVector[--ch_UB]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	this->AVector[--ch_UC]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	this->AVector[--ch_IA]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	this->AVector[--ch_IB]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	this->AVector[--ch_IC]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	complex<double> UA = (polar(AVector[ch_UA]->APhasor.Amplitude,AVector[ch_UA]->APhasor.phase*PI/180));
	complex<double> UB = (polar(AVector[ch_UB]->APhasor.Amplitude,AVector[ch_UB]->APhasor.phase*PI/180));
	complex<double> UC = (polar(AVector[ch_UC]->APhasor.Amplitude,AVector[ch_UC]->APhasor.phase*PI/180));
	complex<double> IA = (polar(AVector[ch_IA]->APhasor.Amplitude,AVector[ch_IA]->APhasor.phase*PI/180));
	complex<double> IB = (polar(AVector[ch_IB]->APhasor.Amplitude,AVector[ch_IB]->APhasor.phase*PI/180));
	complex<double> IC = (polar(AVector[ch_IC]->APhasor.Amplitude,AVector[ch_IC]->APhasor.phase*PI/180));
	switch(Type)
	{
	case 1:
		{
			complex<double> k;
			complex<double> I0 = (IA+IB+IC)/(complex<double> (3,0));
			k=(TempData.z0-TempData.z1)/(TempData.z1*(complex<double> (3,0)));
			z.R1 = std::real(UA/(IA+k*I0));z.X1 = std::imag(UA/(IA+k*I0));
			z.R2 = std::real(UB/(IB+k*I0));z.X2 = std::imag(UB/(IB+k*I0));
			z.R3 = std::real(UC/(IC+k*I0));z.X3 = std::imag(UC/(IC+k*I0));
			break;
		}
	case 2:
		{
			z.R1 = std::real(UA/IA);z.X1 = std::imag(UA/IA);
			z.R2 = std::real(UB/IB);z.X2 = std::imag(UB/IB);
			z.R3 = std::real(UC/IC);z.X3 = std::imag(UC/IC);
			break;
		}
	case 3:
		{
			z.R1 = std::real((UA-UB)/(IA-IB));z.X1 = std::imag((UA-UB)/(IA-IB));
			z.R2 = std::real((UB-UC)/(IB-IC));z.X2 = std::imag((UB-UC)/(IB-IC));
			z.R3 = std::real((UC-UA)/(IC-IA));z.X3 = std::imag((UC-UA)/(IC-IA));
			break;
		}
	}
	z.Z1 = complex<double>(z.R1,z.X1);
	z.Z2 = complex<double>(z.R2,z.X2);
	z.Z3 = complex<double>(z.R3,z.X3);
	return 0;
}


double CComtrade::GetPointTime(int sampPoint)
{
	int size = m_SampleRate.size();
	double time = 0;
	int lastSamp = 0;
	for (int i = 0; i<size; ++i)
	{
		if(sampPoint>m_SampleRate[i].endsamp)
		{
			time += (m_SampleRate[i].endsamp-lastSamp) / m_SampleRate[i].samp;
			lastSamp = m_SampleRate[i].endsamp;
		}
		else if(sampPoint<=m_SampleRate[i].endsamp)
		{
			time += (sampPoint-lastSamp) / m_SampleRate[i].samp;
			break;
		}
	}
	return time*1000;
}


// 获得点通道
int CComtrade::getPointChan(int scrollPos, int pointPos)
{
	int j,i=0;
	int pos=scrollPos+pointPos-m_drawParam.topSpace;
	for(j=0; j<m_ANumber; ++j)
	{
		i+=AVector[j]->m_drawPara.VSpace+m_drawParam.VGap;
		if(i>pos)
			break;
	}
	return j;
}

int CComtrade::getTotalAVSpace(void)
{
	int i=0;
	for(int j=0; j<m_ANumber; ++j)
	{
		i+=AVector[j]->m_drawPara.VSpace;
	}
	i+=m_ANumber*m_drawParam.VGap+m_drawParam.topSpace;
	return i;
}


int CComtrade::getChanPos(int chanNum)
{
	int j,i=0;
	//int top = m_drawParam.topSpace;
	for(j=0; j<chanNum; ++j)
	{
		i+=AVector[j]->m_drawPara.VSpace+m_drawParam.VGap;
	}
	return i;
}


// 母差分析
int CComtrade::GetCurrentBusDiff(int CurrentPoint, int ch_IA1, int ch_IB1, int ch_IC1, int ch_IA2, int ch_IB2, int ch_IC2, int Type)
{
	using std::complex;
	using splab::PI;
	using std::polar;
	using std::arg;
	int Flag = 1;
	Flag = this->AVector[--ch_IA1]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	if (Flag == 1)
	{
		return 1;
	}
	this->AVector[--ch_IB1]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	this->AVector[--ch_IC1]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	this->AVector[--ch_IA2]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	this->AVector[--ch_IB2]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	this->AVector[--ch_IC2]->GetCurrentPhasor(CurrentPoint,this->m_SampleRate);
	complex<double> IA1 = (polar(abs(AVector[ch_IA1]->APhasor.Amplitude),AVector[ch_IA1]->APhasor.phase*PI/180));
	complex<double> IB1 = (polar(abs(AVector[ch_IB1]->APhasor.Amplitude),AVector[ch_IB1]->APhasor.phase*PI/180));
	complex<double> IC1 = (polar(abs(AVector[ch_IC1]->APhasor.Amplitude),AVector[ch_IC1]->APhasor.phase*PI/180));
	complex<double> IA2 = (polar(abs(AVector[ch_IA2]->APhasor.Amplitude),AVector[ch_IA2]->APhasor.phase*PI/180));
	complex<double> IB2 = (polar(abs(AVector[ch_IB2]->APhasor.Amplitude),AVector[ch_IB2]->APhasor.phase*PI/180));
	complex<double> IC2 = (polar(abs(AVector[ch_IC2]->APhasor.Amplitude),AVector[ch_IC2]->APhasor.phase*PI/180));
	//type=1 和差制动
	//type=2 比率制动
	//type=3 标积制动
	dI.rIA = abs(IA1+IA2);
	dI.rIB = abs(IB1+IB2);
	dI.rIC = abs(IC1+IC2);
	if (Type==1)
	{
		dI.resIA = 0.5*abs(IA1-IA2);
		dI.resIB = 0.5*abs(IB1-IB2);
		dI.resIC = 0.5*abs(IC1-IC2);
	}
	else if (Type==2)
	{
		dI.resIA = 0.5*(abs(IA1)+abs(IA2));
		dI.resIB = 0.5*(abs(IB1)+abs(IB2));
		dI.resIC = 0.5*(abs(IC1)+abs(IC2));
	}
	else if (Type==3)
	{
		double dAtheta=abs(arg(IA1/IA2));
		double dBtheta=abs(arg(IB1/IB2));
		double dCtheta=abs(arg(IC1/IC2));
		if (cos(PI-dAtheta)>0)
			dI.resIA =sqrt(cos(PI-dAtheta)*abs(IA1)*abs(IA2));
		else
			dI.resIA =0;
		if (cos(PI-dBtheta)>0)
			dI.resIB =sqrt(cos(PI-dBtheta)*abs(IB1)*abs(IB2));
		else
			dI.resIB =0;
		if (cos(PI-dCtheta)>0)
			dI.resIC =sqrt(cos(PI-dCtheta)*abs(IC1)*abs(IC2));
		else
			dI.resIA =0;
	}
	return 0;
}

int CComtrade::GetPower(int ch_UA, int ch_UB, int ch_UC, int ch_IA, int ch_IB, int ch_IC,CString NAME)
{
	using std::complex;
	using std::polar;
	using splab::PI;
	ch_UA--;ch_UB--;ch_UC--;ch_IA--;ch_IB--;ch_IC--;
	CString Ph[3]={_T("A"),_T("B"),_T("C")};
	CString Name[3]={_T("相有功功率"),_T("相无功功率"),_T("相视在功率")};
	int An[9];
	double sa,sb,sc;
	for(int n=0;n<9;n++)
	{
		An[n]=m_ANumber;
		CString ph = Ph[n/3];
		CString name = NAME + ph+ Name[n%3];
		AVector.push_back(new AChannel(An[n],name,ph,_T(""),_T(""),0,0,0,0,0,0,0,_T(""),0));
		AVector[An[n]]->AData.resize(m_TotalSampleNum);
		AVector[An[n]]->m_drawPara.defultVSpace = m_drawParam.defaultVSpace;
		AVector[An[n]]->m_drawPara.VSpace = m_drawParam.defaultVSpace;

		AVector[An[n]]->maxAData = 0;
		m_ANumber++;
	}
	for (int i=0; i<this->m_TotalSampleNum;i++)
	{
		this->AVector[ch_UA]->GetCurrentPhasor(i+1,this->m_SampleRate);
		this->AVector[ch_UB]->GetCurrentPhasor(i+1,this->m_SampleRate);
		this->AVector[ch_UC]->GetCurrentPhasor(i+1,this->m_SampleRate);
		this->AVector[ch_IA]->GetCurrentPhasor(i+1,this->m_SampleRate);
		this->AVector[ch_IB]->GetCurrentPhasor(i+1,this->m_SampleRate);
		this->AVector[ch_IC]->GetCurrentPhasor(i+1,this->m_SampleRate);
		complex<double> UA = (polar(AVector[ch_UA]->APhasor.Amplitude,AVector[ch_UA]->APhasor.phase*PI/180));
		complex<double> UB = (polar(AVector[ch_UB]->APhasor.Amplitude,AVector[ch_UB]->APhasor.phase*PI/180));
		complex<double> UC = (polar(AVector[ch_UC]->APhasor.Amplitude,AVector[ch_UC]->APhasor.phase*PI/180));
		complex<double> IA = (polar(AVector[ch_IA]->APhasor.Amplitude,AVector[ch_IA]->APhasor.phase*PI/180));
		complex<double> IB = (polar(AVector[ch_IB]->APhasor.Amplitude,AVector[ch_IB]->APhasor.phase*PI/180));
		complex<double> IC = (polar(AVector[ch_IC]->APhasor.Amplitude,AVector[ch_IC]->APhasor.phase*PI/180));
		AVector[An[0]]->AData[i] =abs(UA)*abs(IA)*cos(arg(UA)-arg(IA))/2;
		AVector[An[1]]->AData[i] =abs(UA)*abs(IA)*sin(arg(UA)-arg(IA))/2;
		sa=AVector[An[2]]->AData[i] =sqrt(AVector[An[0]]->AData[i]*AVector[An[0]]->AData[i]+AVector[An[1]]->AData[i]*AVector[An[1]]->AData[i]);
		AVector[An[3]]->AData[i] =abs(UB)*abs(IB)*cos(arg(UB)-arg(IB))/2;
		AVector[An[4]]->AData[i] =abs(UB)*abs(IB)*sin(arg(UB)-arg(IB))/2;
		sb=AVector[An[5]]->AData[i] =sqrt(AVector[An[3]]->AData[i]*AVector[An[3]]->AData[i]+AVector[An[4]]->AData[i]*AVector[An[4]]->AData[i]);
        AVector[An[6]]->AData[i] =abs(UC)*abs(IC)*cos(arg(UC)-arg(IC))/2;
		AVector[An[7]]->AData[i] =abs(UC)*abs(IC)*sin(arg(UC)-arg(IC))/2;
		sc=AVector[An[8]]->AData[i] =sqrt(AVector[An[6]]->AData[i]*AVector[An[6]]->AData[i]+AVector[An[7]]->AData[i]*AVector[An[7]]->AData[i]);
		if (abs(AVector[An[0]]->AData[i])>AVector[An[0]]->maxAData)
			AVector[An[0]]->maxAData = abs(AVector[An[0]]->AData[i]);
		if (abs(AVector[An[1]]->AData[i])>AVector[An[1]]->maxAData)
			AVector[An[1]]->maxAData = abs(AVector[An[1]]->AData[i]);
		if (abs(AVector[An[3]]->AData[i])>AVector[An[3]]->maxAData)
			AVector[An[3]]->maxAData = abs(AVector[An[3]]->AData[i]);
		if (abs(AVector[An[4]]->AData[i])>AVector[An[4]]->maxAData)
			AVector[An[4]]->maxAData = abs(AVector[An[4]]->AData[i]);
		if (abs(AVector[An[6]]->AData[i])>AVector[An[6]]->maxAData)
			AVector[An[6]]->maxAData = abs(AVector[An[6]]->AData[i]);
		if (abs(AVector[An[7]]->AData[i])>AVector[An[7]]->maxAData)
			AVector[An[7]]->maxAData = abs(AVector[An[7]]->AData[i]);
		if (abs(sa)>AVector[An[2]]->maxAData )
			AVector[An[2]]->maxAData =abs(sa);
		if (abs(sb)>AVector[An[5]]->maxAData )
			AVector[An[5]]->maxAData =abs(sb);
		if (abs(sc)>AVector[An[8]]->maxAData )
			AVector[An[8]]->maxAData =abs(sc);
	}
	double Smax = AVector[An[2]]->maxAData;
	if (AVector[An[5]]->maxAData>Smax) Smax = AVector[An[5]]->maxAData;
	if (AVector[An[8]]->maxAData>Smax) Smax = AVector[An[8]]->maxAData;
	for(int n=0;n<9;n++)
	{
		AVector[An[n]]->m_drawPara.UPScale = Smax/(m_drawParam.defaultVSpace/2.0);
		AVector[An[n]]->m_drawPara.defaultUPScale = Smax/(m_drawParam.defaultVSpace/2.0);
	}
	return 0;
}