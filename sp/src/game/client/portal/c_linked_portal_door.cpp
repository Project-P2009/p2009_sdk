#include "cbase.h"
#include "c_linked_portal_door.h"
#include "view_scene.h"
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(linked_portal_door, C_LinkedPortalDoor);

IMPLEMENT_CLIENTCLASS_DT(C_LinkedPortalDoor, DT_LinkedPortalDoor, CLinkedPortalDoor)
END_RECV_TABLE();

C_LinkedPortalDoor::C_LinkedPortalDoor()
	: BaseClass()
{
}

C_LinkedPortalDoor::~C_LinkedPortalDoor()
{
}

extern ConVar mat_wireframe;

ConVar mat_worldportal_uv("mat_worldportal_uv", "0.5 0.5", FCVAR_CLIENTDLL, "World portal UV coordinates.");

void C_LinkedPortalDoor::DrawSimplePortalMesh(const IMaterial *pMaterialOverride, float fForwardOffsetModifier)
{
	const char* szUv = mat_worldportal_uv.GetString();
	Vector2D vecUV = Vector2D(0.5f, 0.5f);
	if (szUv != NULL && Q_strlen(szUv) > 0) {
		sscanf(szUv, "%f%f", &vecUV.x, &vecUV.y);
	}

	const IMaterial *pMaterial;
	if(pMaterialOverride)
		pMaterial = pMaterialOverride;
	else
		pMaterial = m_Materials.m_PortalMaterials[m_bIsPortal2 ? 1 : 0];

	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->Bind((IMaterial *)pMaterial, GetClientRenderable());

	// This can depend on the Bind command above, so keep this after!
	UpdateFrontBufferTexturesForMaterial((IMaterial *)pMaterial);

	pRenderContext->MatrixMode(MATERIAL_MODEL); //just in case
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	Vector ptCenter = m_ptOrigin + (m_vForward * fForwardOffsetModifier);

	Vector verts[4];
	verts[0] = ptCenter + (m_vRight * m_fWidth) - (m_vUp * m_fHeight);
	verts[1] = ptCenter + (m_vRight * m_fWidth) + (m_vUp * m_fHeight);
	verts[2] = ptCenter - (m_vRight * m_fWidth) - (m_vUp * m_fHeight);
	verts[3] = ptCenter - (m_vRight * m_fWidth) + (m_vUp * m_fHeight);

	float vTangent[4] = {-m_vRight.x, -m_vRight.y, -m_vRight.z, 1.0f};

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh(false);
	meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

	meshBuilder.Position3fv(&verts[0].x);
	meshBuilder.TexCoord2f(0, vecUV.x, vecUV.x);
	meshBuilder.TexCoord2f(1, vecUV.x, vecUV.x);
	meshBuilder.Normal3f(m_vForward.x, m_vForward.y, m_vForward.z);
	meshBuilder.UserData(vTangent);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv(&verts[1].x);
	meshBuilder.TexCoord2f(0, vecUV.x, vecUV.x);
	meshBuilder.TexCoord2f(1, vecUV.x, vecUV.x);
	meshBuilder.Normal3f(m_vForward.x, m_vForward.y, m_vForward.z);
	meshBuilder.UserData(vTangent);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv(&verts[2].x);
	meshBuilder.TexCoord2f(0, vecUV.y, vecUV.y);
	meshBuilder.TexCoord2f(1, vecUV.y, vecUV.y);
	meshBuilder.Normal3f(m_vForward.x, m_vForward.y, m_vForward.z);
	meshBuilder.UserData(vTangent);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv(&verts[3].x);
	meshBuilder.TexCoord2f(0, vecUV.y, vecUV.y);
	meshBuilder.TexCoord2f(1, vecUV.y, vecUV.y);
	meshBuilder.Normal3f(m_vForward.x, m_vForward.y, m_vForward.z);
	meshBuilder.UserData(vTangent);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	if(mat_wireframe.GetBool())
	{
		pRenderContext->Bind((IMaterial *)(const IMaterial *)g_pPortalRender->m_MaterialsAccess.m_Wireframe, (CPortalRenderable*)this);

		IMesh* pMesh = pRenderContext->GetDynamicMesh(false);
		meshBuilder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

		meshBuilder.Position3fv(&verts[0].x);
		meshBuilder.TexCoord2f(0, vecUV.x, vecUV.x);
		meshBuilder.TexCoord2f(1, vecUV.x, vecUV.x);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3fv(&verts[1].x);
		meshBuilder.TexCoord2f(0, vecUV.x, vecUV.x);
		meshBuilder.TexCoord2f(1, vecUV.x, vecUV.x);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3fv(&verts[2].x);
		meshBuilder.TexCoord2f(0, vecUV.y, vecUV.y);
		meshBuilder.TexCoord2f(1, vecUV.y, vecUV.y);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3fv(&verts[3].x);
		meshBuilder.TexCoord2f(0, vecUV.y, vecUV.y);
		meshBuilder.TexCoord2f(1, vecUV.y, vecUV.y);
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}

	pRenderContext->MatrixMode(MATERIAL_MODEL);
	pRenderContext->PopMatrix();
}