#pragma once
#include "Scene.h"
class CMidBoss :
	public CScene
{
public:
	CMidBoss();
	virtual ~CMidBoss();

	// CScene��(��) ���� ��ӵ�
	virtual void Initialize() override;
	virtual void LateInit() override;
	virtual void Update() override;
	virtual void LateUpdate() override;
	virtual void Render(HDC hDC) override;
	virtual void Release() override;

private:
	int m_iBackgroundIdx;
	TCHAR* m_pBackgroundKey[4];
	DWORD m_dwBackgroundTime;
};
