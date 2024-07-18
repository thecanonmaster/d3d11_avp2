struct NodePath
{
	uint8	m_anPath[4]; // 0

	uint32	m_nLevel; // 4
};

struct TerrainSection 
{
	uint32*	m_pVTable; // 0

	char	m_szWorldName[MAX_WORLDNAME_LEN + 1]; // 4
	uint8	m_nWorldNamePadding0; // 69
	uint16	m_wWorldNamePadding1; // 70

	NodePath	m_NodePath; // 72

	CMoArray<WorldPoly*, DefaultCache>	m_Polies; // 80;

	Node*	m_pRootNode; // 100

	CMoArray<Node, DefaultCache>	m_Nodes; // 104

	WorldBsp*	m_pBaseBSP; // 124

	PBlockTable	m_PBlockTable; // 128

	LTVector	m_vData_176; // 176

	float	m_fPolyClipRadius; // 188

	// 192 max
};
