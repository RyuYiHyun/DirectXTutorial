/*
*	포함 디렉토리 : C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include 추가
*	라이브러리 디렉토리 : C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Lib\x86 추가
*/
#pragma comment (lib,"d3d9.lib")
#pragma comment (lib,"d3dx9.lib")
#pragma comment (lib,"winmm.lib")
#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
LPDIRECT3D9             g_pD3D = NULL;			// Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL;	// Our rendering device
int state = 0;				// 상태 (1 우주 대각선으로 보기,2 우주선에서 보기)
float angle = 0;			// 각도
D3DVECTOR SeeVector;		// 우주선의 바라보는 벡터
HWND g_hWnd;				
#define MAX_SPHERE 10		// 구 개수

#pragma region SolarSystemTexture
// 택스처 배열
LPDIRECT3DTEXTURE9 g_pTexture[MAX_SPHERE];
const wchar_t* g_textureMapName[] =
{// 태양, 수성, 금성, 지구, 달, 화성, 목성, 토성, 천왕성, 해왕성
	L"SunMap.jpg",L"MercuryMap.jpg",L"VenusMap.jpg",L"EarthMap.jpg",L"MoonMap.jpg",
	L"MarsMap.jpg", L"JupiterMap.jpg",L"SaturnMap.jpg",L"UranusMap.jpg",L"NeptuneMap.jpg"
};
// 우주 택스처
LPDIRECT3DTEXTURE9 g_GalaxyTexture;
const wchar_t* g_GalaxyName = L"GalaxyMap.jpg";
#pragma endregion

class Object
{
public:
	// 크기 레벨
	float sizeLevel = 1.0f;
	// 정점 정보
	LPDIRECT3DVERTEXBUFFER9 pVB;
	// 매쉬 정보
	LPD3DXMESH pMesh;
	// 크기 행렬
	D3DXMATRIXA16 Scale;
	// 자전 행렬
	D3DXMATRIXA16 Rotate;
	// 자전 각
	D3DVECTOR RotateAngle;
	// 이동 행렬
	D3DXMATRIXA16 Translation;
	// 부모
	Object* Parents;
	// 공전 행렬
	D3DXMATRIXA16 Orbital;
	// 공전 각
	D3DVECTOR OrbitalAngle;

	Object()
	{
		pVB = NULL;
		pMesh = NULL;
		D3DXMatrixIdentity(&Scale);
		D3DXMatrixIdentity(&Rotate);
		D3DXMatrixIdentity(&Translation);
		D3DXMatrixIdentity(&Orbital);
		Parents = nullptr;
		RotateAngle.x = 0;
		RotateAngle.y = 0;
		RotateAngle.z = 0;
		OrbitalAngle.x = 0;
		OrbitalAngle.y = 0;
		OrbitalAngle.z = 0;
	}
private:
};
Object* g_Sphere[MAX_SPHERE];
Object* g_Cube;
Object* g_GalaxyBox;

struct CUSTOMVERTEX
{
	D3DXVECTOR3 position;	// 좌표
	D3DXVECTOR3 normal;		// 노말 벡터
	float tu;				// 텍스처 좌표
	float tv;
	//   0,0 --- 1,0 U
	//
	//
	//   0,1 --- 1,1
	// V
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

HRESULT InitD3D(HWND hWnd)
{
	// Create the D3D object.
	if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		return E_FAIL;
	// Set up the structure used to create the D3DDevice. Since we are now
	// using more complex geometry, we will create a device with a zbuffer.
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	// Create the D3DDevice
	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}

	// Turn off culling
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// Turn on the zbuffer
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);


	return S_OK;
}
// 구에 대한 UV 좌표 변환
void convert(float x, float y, float z, float r, float& u, float& v) {
	float q;
	float q2;
	q = atan2(z, x);

	u = q / (2.0f * D3DX_PI);
	q2 = asin(y / r);
	v = (1.0f - q2 / (D3DX_PI / 2.0f)) / 2.0f;
	if (u > 1.0)
	{
		u = 1.0;
	}
}
HRESULT InitGeometry()
{
	for (int i = 0; i < MAX_SPHERE; i++)
	{
		if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice, g_textureMapName[i], &g_pTexture[i])))
		{
			wchar_t tempstr[256];
			ZeroMemory(tempstr, sizeof(tempstr));
			const wchar_t* conststring = L"..\\";
			wcscat(tempstr, conststring);
			wcscat(tempstr, g_textureMapName[i]);
			// 현제 폴더에 파일이 없으면 상위 폴더 검색
			if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice, tempstr, &g_pTexture[i])))
			{
				MessageBox(NULL, L"Could not find EarthMap.jpg", L"Ryu.exe", MB_OK);
				return E_FAIL;
			}
		}
	}

#pragma region 구 오브젝트 정보 초기화

	for (int i = 0; i < MAX_SPHERE; i++)
	{
		g_Sphere[i] = new Object();
	}
	g_Sphere[0]->sizeLevel = 50.0f;
	g_Sphere[1]->sizeLevel = 0.4f;
	g_Sphere[2]->sizeLevel = 0.9f;
	g_Sphere[3]->sizeLevel = 1.0f;
	g_Sphere[4]->sizeLevel = 0.25f;
	g_Sphere[5]->sizeLevel = 0.5f;
	g_Sphere[6]->sizeLevel = 11.2f;
	g_Sphere[7]->sizeLevel = 9.4f;
	g_Sphere[8]->sizeLevel = 4.0f;
	g_Sphere[9]->sizeLevel = 3.9f;
	//화 0.5 목11.2 토 9.4  천왕 4.0 해 3.9

	for (int i = 0; i < MAX_SPHERE; i++)
	{
		LPD3DXMESH tempMesh = NULL;
		
		D3DXCreateSphere(g_pd3dDevice, g_Sphere[i]->sizeLevel, 64, 32, &tempMesh, NULL);

		tempMesh->CloneMeshFVF(NULL, D3DFVF_CUSTOMVERTEX, g_pd3dDevice, &g_Sphere[i]->pMesh);
		LPDIRECT3DVERTEXBUFFER9 pVB = NULL;
		CUSTOMVERTEX* pVer = NULL;
		g_Sphere[i]->pMesh->GetVertexBuffer(&pVB);
		pVB->Lock(0, 0, (VOID**)&pVer, 0);
		DWORD n;
		for (n = 0; n < g_Sphere[i]->pMesh->GetNumVertices(); n++) {
			float u, v;
			convert(pVer[n].position.x, pVer[n].position.y, pVer[n].position.z, g_Sphere[i]->sizeLevel, u, v);
			pVer[n].tu = u;
			pVer[n].tv = v;
		}
		pVB->Unlock();
	}


	// 태양 설정
	//D3DXMatrixScaling(&g_Sphere[0]->Scale, 10.0f, 10.0f, 10.0f);// 약10
	// 행성 설정
	//D3DXMatrixScaling(&g_Sphere[1]->Scale, 1.0f, 1.0f, 1.0f);// 수성 0.38
	D3DXMatrixTranslation(&g_Sphere[1]->Translation, 50.0f + 4.0f, 0.0f, 0.0f);

	//D3DXMatrixScaling(&g_Sphere[2]->Scale, 2.0f, 2.0f, 2.0f);// 금성 0.72
	D3DXMatrixTranslation(&g_Sphere[2]->Translation, 50.0f + 7.0f, 0.0f, 0.0f);

	//D3DXMatrixScaling(&g_Sphere[3]->Scale, 3.0f, 3.0f, 3.0f);// 지구 1
	D3DXMatrixTranslation(&g_Sphere[3]->Translation, 50.0f + 10.0f, 0.0f, 0.0f);

	//D3DXMatrixScaling(&g_Sphere[4]->Scale, 1.0f, 1.0f, 1.0f);// 달
	D3DXMatrixTranslation(&g_Sphere[4]->Translation, 1.5f, 0.0f, 0.0f);
	g_Sphere[4]->Parents = g_Sphere[3];

	D3DXMatrixTranslation(&g_Sphere[5]->Translation, 50.0f + 15.0f, 0.0f, 0.0f);//화성 1.5
	D3DXMatrixTranslation(&g_Sphere[6]->Translation, 50.0f + 52.0f, 0.0f, 0.0f);//목 5.2
	D3DXMatrixTranslation(&g_Sphere[7]->Translation, 50.0f + 95.4f, 0.0f, 0.0f);// 토 9.54
	D3DXMatrixTranslation(&g_Sphere[8]->Translation, 50.0f + 192.0f, 0.0f, 0.0f);// 천 19.2
	D3DXMatrixTranslation(&g_Sphere[9]->Translation, 50.0f + 301.0f, 0.0f, 0.0f);// 해 30.1
	

#pragma endregion

	{
		g_Cube = new Object();
		LPD3DXMESH tempMesh = NULL;// 임시 매쉬
		D3DXCreateBox(g_pd3dDevice, 1.0f, 1.0f, 1.0f, &tempMesh, NULL);
		g_Cube->pMesh = tempMesh;

		D3DXMatrixScaling(&g_Cube->Scale, 1.0f, 1.0f, 1.0f);// 약10
		D3DXMatrixTranslation(&g_Cube->Translation, 0.0f, 0.0f, -250.0f);
	}

	{
		g_GalaxyBox = new Object();
		LPD3DXMESH tempMesh = NULL;// 임시 매쉬
		D3DXCreateBox(g_pd3dDevice, 2000.0f, 2000.0f, 2000.0f, &tempMesh, NULL);
		tempMesh->CloneMeshFVF(NULL, D3DFVF_CUSTOMVERTEX, g_pd3dDevice, &g_GalaxyBox->pMesh);
		LPDIRECT3DVERTEXBUFFER9 pVB = NULL;
		CUSTOMVERTEX* pVer = NULL;
		g_GalaxyBox->pMesh->GetVertexBuffer(&pVB);
		pVB->Lock(0, 0, (VOID**)&pVer, 0);
		DWORD n;
		DWORD end = g_GalaxyBox->pMesh->GetNumVertices();
		for (n = 0; n < g_GalaxyBox->pMesh->GetNumVertices(); n++) {
			//(0,0)--------(1,0)
			//
			//
			//
			//(0,1)--------(1,1)
			// (0,0) -> (1,0) -> (0,1) -> (1,1)
			switch (n)
			{
			case 20:
			case 17:
			case 0:
			case 8:
			case 4:
			case 12:
				pVer[n].tu = 0;
				pVer[n].tv = 0.25;//0.25;
				break;
			case 21:
			case 18:
			case 1:
			case 9:
			case 5:
			case 13:
				pVer[n].tu = 0;
				pVer[n].tv = 0;
				break;
			case 22:
			case 19:
			case 2:
			case 10:
			case 6:
			case 14:
				pVer[n].tu = 0.25;// 0.25;
				pVer[n].tv = 0;
				break;
			case 23:
			case 16:
			case 3:
			case 11:
			case 7:
			case 15:
				pVer[n].tu = 0.25;//0.25;
				pVer[n].tv = 0.25;//0.25;
				break;
			}
		}
		pVB->Unlock();
		D3DXMatrixTranslation(&g_GalaxyBox->Translation, 200.0f, 0.0f, 0.0f);
	}
	if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice, g_GalaxyName, &g_GalaxyTexture)))
	{
		wchar_t tempstr[256];
		ZeroMemory(tempstr, sizeof(tempstr));
		const wchar_t* conststring = L"..\\";
		wcscat(tempstr, conststring);
		wcscat(tempstr, g_GalaxyName);
		// 현제 폴더에 파일이 없으면 상위 폴더 검색
		if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice, tempstr, &g_GalaxyTexture)))
		{
			MessageBox(NULL, L"Could not find.jpg", L"Ryu.exe", MB_OK);
			return E_FAIL;
		}
	}


	// 보는 각
	SeeVector.x = 0;
	SeeVector.y = 0;
	SeeVector.z = 1;

	return S_OK;
}

VOID Cleanup()
{
	for (int i = 0; i < MAX_SPHERE; i++)
	{
		if (g_Sphere[i]->pVB != NULL)
			g_Sphere[i]->pVB->Release();
	}

	if (g_pd3dDevice != NULL)
		g_pd3dDevice->Release();

	if (g_pD3D != NULL)
		g_pD3D->Release();
}

VOID SetupObjectMatrices(Object* oject)
{
	// Set up world matrix
	D3DXMATRIXA16 matWorld;
	D3DXMATRIXA16 tempX;
	D3DXMATRIXA16 tempY;
	D3DXMATRIXA16 tempZ;
	D3DXMatrixIdentity(&matWorld);

	D3DXMatrixRotationX(&tempX, oject->RotateAngle.x);
	D3DXMatrixRotationY(&tempY, oject->RotateAngle.y);
	D3DXMatrixRotationZ(&tempZ, oject->RotateAngle.z);
	oject->Rotate = tempX * tempY * tempZ;

	D3DXMatrixRotationX(&tempX, oject->OrbitalAngle.x);
	D3DXMatrixRotationY(&tempY, oject->OrbitalAngle.y);
	D3DXMatrixRotationZ(&tempZ, oject->OrbitalAngle.z);
	oject->Orbital = tempX * tempY * tempZ;

	if (oject->Parents != nullptr)
	{
		matWorld = (oject->Scale * oject->Rotate * oject->Translation * oject->Orbital) * oject->Parents->Translation * oject->Parents->Orbital;
	}
	else
	{
		matWorld = oject->Scale * oject->Rotate * oject->Translation * oject->Orbital;
	}

	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
}

VOID SetupCamara()
{
	if (state == 0)
	{
		D3DXVECTOR3 vEyePt(0.0f, 500.0f, -1000.0f);
		D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
		D3DXMATRIXA16 matView;
		D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
		g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);


		D3DXMATRIXA16 matProj;
		D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 3000.0f);
		g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
	}
	else if (state == 1)
	{
		D3DXVECTOR3 vEyePt(g_Cube->Translation.m[3][0], g_Cube->Translation.m[3][1], g_Cube->Translation.m[3][2]);
		D3DXVECTOR3 vLookatPt(g_Cube->Translation.m[3][0] + SeeVector.x, g_Cube->Translation.m[3][1] + SeeVector.y, g_Cube->Translation.m[3][2] + SeeVector.z);
		D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
		D3DXMATRIXA16 matView;
		D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
		g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);


		D3DXMATRIXA16 matProj;
		D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 2000.0f);
		g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
	}

}

VOID SetupMaterial(bool emissive)
{
	D3DMATERIAL9 mtrl;
	ZeroMemory(&mtrl, sizeof(D3DMATERIAL9));
	mtrl.Ambient.r = 0.5f;
	mtrl.Ambient.g = 0.5f;
	mtrl.Ambient.b = 0.5f;
	mtrl.Ambient.a = 1.0f;

	mtrl.Diffuse.r = 1.0f;
	mtrl.Diffuse.g = 1.0f;
	mtrl.Diffuse.b = 1.0f;
	mtrl.Diffuse.a = 1.0f;

	if (emissive) // 자체 발광 값
	{
		mtrl.Emissive.r = 1.0f;
		mtrl.Emissive.g = 1.0f;
		mtrl.Emissive.b = 1.0f;
		mtrl.Emissive.a = 1.0f;
	}
	g_pd3dDevice->SetMaterial(&mtrl);
}
VOID SetupLights()
{
	/*
	인터넷 검색
	Ambient Light (주변광) : 수많은 반사를 거쳐서 광원이 불분명한 빛이다.
						   물체를 덮고 있는 빛이며 일정한 밝기와 색으로 표현된다.
						   ex) 물체색 * 주변광 , 주변광이 어두우면 물체색에서 어둡게 반대면 반대로
	Diffuse Light (분산광) : 물체의 표면에서 분산되어 눈으로 바로 들어오는 빛이다.
						  각도에 따라 밝기가 다르다.
	Specular Light (반사광) : 분산광과 달리 한방향으로 완전히 반사되는 빛이다.
							반사되는 부분은 흰색의 광으로 보인다.

	교수님 설명
	diffuse : (난반사) 물체의 색으로 하자, 물체의 고유의 색보다 반사되어 우리 눈에 인식되는 색 / 최종(물체가 가지는 색)
	emit : (발산광) 물체가 조명처럼 기능하는 색 / 최종(물체 발산하는 색)
	specular : (정반사) / 최종(반짝거림, 하이라이트)
	Ambient : 주변광을 일일이 계산하기에는 과부하가 심하기 때문에 한번에 퉁치기
	*/
	


	D3DXVECTOR3 vecDir;
	D3DLIGHT9 light;
	ZeroMemory(&light, sizeof(D3DLIGHT9));
	///*
	light.Type = D3DLIGHT_POINT;
	light.Diffuse.r = 1.0f;
	light.Diffuse.g = 1.0f;
	light.Diffuse.b = 1.0f;
	light.Attenuation0 = 0.00000001f;
	light.Range = 10000;
	light.Position.x = 0.0f;
	light.Position.y = 0.0f;
	light.Position.z = 0.0f;
	//*/

	/*
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse.r = 1.0f;
	light.Diffuse.g = 1.0f;
	light.Diffuse.b = 1.0f;
	light.Specular.r = light.Specular.g = light.Specular.b = 0.2f;
	light.Ambient.r = light.Ambient.g = light.Ambient.b = 0.5f;
	// 빛 고정
	vecDir = D3DXVECTOR3(1.0f, -2.0f, 1.0f);

	D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecDir);
	light.Range = 1000.0f;
	//*/

	g_pd3dDevice->SetLight(0, &light);
	g_pd3dDevice->LightEnable(0, TRUE);
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	
	// Finally, turn on some ambient light.
	g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, 0x00202020);
}

VOID Input()
{
	static float speed = 1.5f;
	if (GetKeyState(VK_NUMPAD1) & 0x8000)
	{
		state = 0;
	}
	if (GetKeyState(VK_NUMPAD2) & 0x8000)
	{
		state = 1;
	}
	if (GetKeyState(VK_SPACE) & 0x8000)
	{
		D3DXMatrixTranslation(&g_Cube->Translation, 0.0f, 0.0f, -250.0f);
	}
	if (state == 1)
	{
		POINT point;
		RECT rect;
		GetWindowRect(g_hWnd, &rect);
		GetCursorPos(&point);
		point.x -= rect.left;
		point.y -= rect.top;
		if (point.x < 200)
		{
			angle -= 0.5;
			if (angle <= -360)
			{
				angle = 0;
			}
		}
		if (point.x > 700)
		{
			angle += 0.5;
			if (angle > 360)
			{
				angle = 0;
			}
		}
		SeeVector.x = sin(angle * D3DX_PI / 180);
		SeeVector.z = cos(angle * D3DX_PI / 180);
	}

	if (GetKeyState(VK_LEFT) & 0x8000 || GetKeyState('A') & 0x8000)
	{
		D3DXMATRIXA16 temp;
		D3DXMatrixTranslation(&temp, -SeeVector.z * speed, 0.0f, SeeVector.x * speed);
		g_Cube->Translation = g_Cube->Translation * temp;
	}
	if (GetKeyState(VK_RIGHT) & 0x8000 || GetKeyState('D') & 0x8000)
	{
		D3DXMATRIXA16 temp;
		D3DXMatrixTranslation(&temp, SeeVector.z * speed, 0.0f, -SeeVector.x * speed);
		g_Cube->Translation = g_Cube->Translation * temp;
	}
	if (GetKeyState(VK_UP) & 0x8000 || GetKeyState('W') & 0x8000)
	{
		D3DXMATRIXA16 temp;
		D3DXMatrixTranslation(&temp, SeeVector.x * speed, 0.0f, SeeVector.z * speed);
		g_Cube->Translation = g_Cube->Translation * temp;
	}
	if (GetKeyState(VK_DOWN) & 0x8000 || GetKeyState('S') & 0x8000)
	{
		D3DXMATRIXA16 temp;
		D3DXMatrixTranslation(&temp, -SeeVector.x * speed, 0.0f, -SeeVector.z * speed);
		g_Cube->Translation = g_Cube->Translation * temp;
	}
}

VOID Update()
{
	g_Sphere[0]->RotateAngle.y = timeGetTime() / 15000.0f;
	g_Sphere[1]->RotateAngle.y = timeGetTime() / 30000.0f;
	g_Sphere[2]->RotateAngle.y = timeGetTime() / 120000.0f;
	g_Sphere[3]->RotateAngle.y = timeGetTime() / 1000.0f;
	g_Sphere[4]->RotateAngle.y = timeGetTime() / 15000.0f;
	g_Sphere[5]->RotateAngle.y = timeGetTime() / 1000.0f;
	g_Sphere[6]->RotateAngle.y = timeGetTime() / 500.0f;
	g_Sphere[7]->RotateAngle.y = timeGetTime() / 500.0f;
	g_Sphere[8]->RotateAngle.y = timeGetTime() / 800.0f;
	g_Sphere[9]->RotateAngle.y = timeGetTime() / 800.0f;
	///*
	g_Sphere[1]->OrbitalAngle.y = timeGetTime() / 500.0f;//수88일
	g_Sphere[2]->OrbitalAngle.y = timeGetTime() / 800.0f;//금255일
	g_Sphere[3]->OrbitalAngle.y = timeGetTime() / 1200.0f;//지365일
	g_Sphere[4]->OrbitalAngle.y = timeGetTime() / 500.0f;//달 30일
	g_Sphere[5]->OrbitalAngle.y = timeGetTime() / 2000.0f;//화687일
	g_Sphere[6]->OrbitalAngle.y = timeGetTime() / 8000.0f;//목11.9년 = 4380
	g_Sphere[7]->OrbitalAngle.y = timeGetTime() / 20000.0f;//토29.5년 = 10700
	g_Sphere[8]->OrbitalAngle.y = timeGetTime() / 40000.0f;//천84년 = 30000
	g_Sphere[9]->OrbitalAngle.y = timeGetTime() / 80000.0f;//해165년 = 60000
	//*/
}

VOID Render()
{
	// Clear the backbuffer and the zbuffer
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);
	SetupLights();
	SetupCamara();

	for (int i = 0; i < MAX_SPHERE; i++)
	{
		if (i == 0)
		{
			SetupMaterial(true);
		}
		else
		{
			SetupMaterial(false);
		}
		
		SetupObjectMatrices(g_Sphere[i]);
		///*
		g_pd3dDevice->SetTexture(0, g_pTexture[i]);
		g_pd3dDevice->SetRenderState(D3DRS_WRAP0, D3DWRAP_U);// 택스처 맵핑 방법 (구에서 택스처 보간)
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		
		//*/
		if (SUCCEEDED(g_pd3dDevice->BeginScene()))
		{
			g_Sphere[i]->pMesh->DrawSubset(0);
			/// 렌더링 종료, 폴리곤을 다 그렸다고 D3D에게 알림(EndScene).
			g_pd3dDevice->EndScene();
		}
	}
	SetupObjectMatrices(g_Cube);
	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		g_Cube->pMesh->DrawSubset(0);
		/// 렌더링 종료, 폴리곤을 다 그렸다고 D3D에게 알림(EndScene).
		g_pd3dDevice->EndScene();
	}
	SetupObjectMatrices(g_GalaxyBox);
	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		SetupMaterial(true);
		g_pd3dDevice->SetTexture(0, g_GalaxyTexture);//g_pTexture[3]
		g_GalaxyBox->pMesh->DrawSubset(0);
		/// 렌더링 종료, 폴리곤을 다 그렸다고 D3D에게 알림(EndScene).
		g_pd3dDevice->EndScene();
	}
	// Present the backbuffer contents to the display
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

INT WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, INT)
{
	UNREFERENCED_PARAMETER(hInst);

	// Register the window class
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		L"D3D Tutorial", NULL
	};
	RegisterClassEx(&wc);

	// Create the application's window
	HWND hWnd = CreateWindow(L"D3D Tutorial", L"D3D Tutorial : SolarSystem",
		WS_OVERLAPPEDWINDOW, 100, 100, 900, 900,
		NULL, NULL, wc.hInstance, NULL);
	g_hWnd = hWnd;
	// Initialize Direct3D
	if (SUCCEEDED(InitD3D(hWnd)))
	{
		// Create the geometry
		if (SUCCEEDED(InitGeometry()))
		{
			// Show the window
			ShowWindow(hWnd, SW_SHOWDEFAULT);
			UpdateWindow(hWnd);

			// Enter the message loop
			MSG msg;
			ZeroMemory(&msg, sizeof(msg));
			while (msg.message != WM_QUIT)
			{
				if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					Input();
					Update();
					Render();
				}
			}
		}
	}

	UnregisterClass(L"D3D Tutorial", wc.hInstance);
	return 0;
}