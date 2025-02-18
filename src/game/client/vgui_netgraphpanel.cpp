//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "inetgraphpanel.h"
#include "kbutton.h"
#include <inetchannelinfo.h>
#include "input.h"
#include <vgui/IVGui.h>
#include "VGuiMatSurface/IMatSystemSurface.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include "tier0/vprof.h"
#include "tier0/cpumonitoring.h"
#include "cdll_bounded_cvars.h"

#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static ConVar	net_scale			( "net_scale", "5", FCVAR_ARCHIVE );
static ConVar	net_graphpos		( "net_graphpos", "1", FCVAR_ARCHIVE );
static ConVar	net_graphsolid		( "net_graphsolid", "1", FCVAR_ARCHIVE );
static ConVar	net_graphtext		( "net_graphtext", "1", FCVAR_ARCHIVE, "Draw text fields" );
static ConVar	net_graphmsecs		( "net_graphmsecs", "400", FCVAR_ARCHIVE, "The latency graph represents this many milliseconds." );
static ConVar	net_graphshowlatency( "net_graphshowlatency", "1", FCVAR_ARCHIVE, "Draw the ping/packet loss graph." );
static ConVar	net_graphshowinterp ( "net_graphshowinterp", "1", FCVAR_ARCHIVE, "Draw the interpolation graph." );

void NetgraphFontChangeCallback( IConVar *var, const char *pOldValue, float flOldValue );

static ConVar	net_graph			( "net_graph","0", 0, "Draw the network usage graph, = 2 draws data on payload, = 3 draws payload legend.", NetgraphFontChangeCallback );
static ConVar	net_graphheight		( "net_graphheight", "64", FCVAR_ARCHIVE, "Height of netgraph panel", NetgraphFontChangeCallback );
static ConVar	net_graphproportionalfont( "net_graphproportionalfont", "1", FCVAR_ARCHIVE, "Determines whether netgraph font is proportional or not", NetgraphFontChangeCallback );


#define	TIMINGS	1024       // Number of values to track (must be power of 2) b/c of masking
#define FRAMERATE_AVG_FRAC 0.9
#define PACKETLOSS_AVG_FRAC 0.5
#define PACKETCHOKE_AVG_FRAC 0.5

#define NUM_LATENCY_SAMPLES 8

#define GRAPH_RED	(0.9 * 255)
#define GRAPH_GREEN (0.9 * 255)
#define GRAPH_BLUE	(0.7 * 255)

#define LERP_HEIGHT 24

#define COLOR_DROPPED	0
#define COLOR_INVALID	1
#define COLOR_SKIPPED	2
#define COLOR_CHOKED	3
#define COLOR_NORMAL	4

//-----------------------------------------------------------------------------
// Purpose: Displays the NetGraph 
//-----------------------------------------------------------------------------
class CNetGraphPanel : public Panel
{
	typedef Panel BaseClass;
private:
	typedef struct
	{
		int latency;
		int	choked;
	} packet_latency_t;

	typedef struct
	{
		unsigned short msgbytes[INetChannelInfo::TOTAL+1];
		int				sampleY;
		int				sampleHeight;

	} netbandwidthgraph_t;

	typedef struct
	{
		 float		cmd_lerp;
		int			size;
		bool		sent;
	} cmdinfo_t;

	typedef struct
	{
		byte color[3];
		byte alpha;
	} netcolor_t;

	byte colors[ LERP_HEIGHT ][3];

	byte sendcolor[ 3 ];
	byte holdcolor[ 3 ];
	byte extrap_base_color[ 3 ];

	packet_latency_t	m_PacketLatency[ TIMINGS ];
	cmdinfo_t			m_Cmdinfo[ TIMINGS ];
	netbandwidthgraph_t	m_Graph[ TIMINGS ];

	float	m_Framerate;
	float   m_AvgLatency;
	float	m_AvgPacketLoss;
	float	m_AvgPacketChoke;
	int		m_IncomingSequence;
	int		m_OutgoingSequence;
	int		m_UpdateWindowSize;
	float	m_IncomingData;
	float	m_OutgoingData;
	float	m_AvgPacketIn;
	float	m_AvgPacketOut;

	int		m_StreamRecv[MAX_FLOWS];
	int		m_StreamTotal[MAX_FLOWS];

	netcolor_t netcolors[5];

	HFont			m_hFontProportional;
	HFont			m_hFont;

	HFont			m_hFontSmall;
	const ConVar_ServerBounded	*cl_updaterate;
	const ConVar_ServerBounded	*cl_cmdrate;

public:
						CNetGraphPanel( VPANEL parent );
	virtual				~CNetGraphPanel( void );

	virtual void		ApplySchemeSettings(IScheme *pScheme);
	virtual void		Paint();
	virtual void		OnTick( void );

	virtual bool		ShouldDraw( void );

	void				InitColors( void );
	int					GraphValue( void );

	struct CLineSegment
	{
		int			x1, y1, x2, y2;
		byte		color[4];
		byte		color2[4];
	};

	CUtlVector< CLineSegment >	m_Rects;

	inline void			DrawLine( vrect_t *rect, unsigned char *color, unsigned char alpha );
	inline void			DrawLine2( vrect_t *rect, unsigned char *color, unsigned char *color2, unsigned char alpha, unsigned char alpha2 );

	void				ResetLineSegments();
	void				DrawLineSegments();

	int					DrawDataSegment( vrect_t *rcFill, int bytes, byte r, byte g, byte b, byte alpha = 255);
	void				DrawUpdateRate( int xright, int y );
	void				DrawCmdRate( int xright, int y );
	void				DrawHatches( int x, int y, int maxmsgbytes );
	void				DrawStreamProgress( int x, int y, int width );
	void				DrawTimes( vrect_t vrect, cmdinfo_t *cmdinfo, int x, int w, int graphtype );
	void				DrawTextFields( int graphvalue, int x, int y, int w, netbandwidthgraph_t *graph, cmdinfo_t *cmdinfo );
	void				GraphGetXY( vrect_t *rect, int width, int *x, int *y );
	void				GetCommandInfo( INetChannelInfo *netchannel, cmdinfo_t *cmdinfo );
	void				GetFrameData( INetChannelInfo *netchannel, int *biggest_message, float *avg_message, float *f95thpercentile );
	void				ColorForHeight( packet_latency_t *packet, byte *color, int *ping, byte *alpha );
	void				GetColorValues( int color, byte *cv, byte *alpha );

	void				OnFontChanged();

private:

	void				PaintLineArt( int x, int y, int w, int graphtype, int maxmsgbytes );
	void				DrawLargePacketSizes( int x, int w, int graphtype, float warning_threshold );

	HFont			GetNetgraphFont()
	{
		return net_graphproportionalfont.GetBool() ? m_hFontProportional : m_hFont;
	}

	void				ComputeNetgraphHeight();
	void				UpdateEstimatedServerFramerate( INetChannelInfo *netchannel );

	CMaterialReference	m_WhiteMaterial;

	int m_EstimatedWidth;

	int					m_nNetGraphHeight;

	float				m_flServerFramerate;
	float				m_flServerFramerateStdDeviation;
};

CNetGraphPanel *g_pNetGraphPanel = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CNetGraphPanel::CNetGraphPanel( VPANEL parent )
: BaseClass( NULL, "CNetGraphPanel" )
{
	int w, h;
	surface()->GetScreenSize( w, h );

	SetParent( parent );
	SetSize( w, h );
	SetPos( 0, 0 );
	SetVisible( false );
	SetCursor( null );

	m_hFont = 0;
	m_hFontProportional = 0;
	m_hFontSmall = 0;
	m_EstimatedWidth = 1;
	m_nNetGraphHeight = 100;

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );

	InitColors();

	cl_updaterate = static_cast< const ConVar_ServerBounded* >( cvar->FindVar( "cl_updaterate" ) );
	cl_cmdrate = static_cast< const ConVar_ServerBounded* >( cvar->FindVar( "cl_cmdrate" ) );
	assert( cl_updaterate && cl_cmdrate );

	memset( sendcolor, 0, 3 );
	memset( holdcolor, 0, 3 );
	sendcolor[ 0 ] = sendcolor[ 1 ] = 255;

	memset( extrap_base_color, 255, 3 );

	memset( m_PacketLatency, 0, TIMINGS * sizeof( packet_latency_t ) );
	memset( m_Cmdinfo, 0, TIMINGS * sizeof( cmdinfo_t ) );
	memset( m_Graph, 0, TIMINGS * sizeof( netbandwidthgraph_t ) );

	m_Framerate = 0.0f;
	m_AvgLatency = 0.0f;
	m_AvgPacketLoss = 0.0f;
	m_AvgPacketChoke = 0.0f;
	m_IncomingSequence = 0;
	m_OutgoingSequence = 0;
	m_UpdateWindowSize = 0;
	m_IncomingData = 0;
	m_OutgoingData = 0;
	m_AvgPacketIn = 0.0f;
	m_AvgPacketOut = 0.0f;
	m_flServerFramerate = 0;
	m_flServerFramerateStdDeviation = 0;

	netcolors[COLOR_DROPPED].color[0] = 255;
	netcolors[COLOR_DROPPED].color[1] = 0;
	netcolors[COLOR_DROPPED].color[2] = 0;
	netcolors[COLOR_DROPPED].alpha = 255;
	netcolors[COLOR_INVALID].color[0] = 0;
	netcolors[COLOR_INVALID].color[1] = 0;
	netcolors[COLOR_INVALID].color[2] = 255;
	netcolors[COLOR_INVALID].alpha = 255;
	netcolors[COLOR_SKIPPED].color[0] = 240;
	netcolors[COLOR_SKIPPED].color[1] = 127;
	netcolors[COLOR_SKIPPED].color[2] = 63;
	netcolors[COLOR_SKIPPED].alpha = 255;
	netcolors[COLOR_CHOKED].color[0] = 225;
	netcolors[COLOR_CHOKED].color[1] = 225;
	netcolors[COLOR_CHOKED].color[2] = 0;
	netcolors[COLOR_CHOKED].alpha = 255;
	netcolors[COLOR_NORMAL].color[0] = 63;
	netcolors[COLOR_NORMAL].color[1] = 255;
	netcolors[COLOR_NORMAL].color[2] = 63;
	netcolors[COLOR_NORMAL].alpha = 232;

	ivgui()->AddTickSignal( GetVPanel(), 500 );

	m_WhiteMaterial.Init( "vgui/white", TEXTURE_GROUP_OTHER );
	g_pNetGraphPanel = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetGraphPanel::~CNetGraphPanel( void )
{
	g_pNetGraphPanel = NULL;
}

void NetgraphFontChangeCallback( IConVar *var, const char *pOldValue, float flOldValue )
{
	if ( g_pNetGraphPanel )
	{
		g_pNetGraphPanel->OnFontChanged();
	}
}

void CNetGraphPanel::OnFontChanged()
{
	// Estimate the width of our panel.
	char str[512];
	wchar_t ustr[512];
	Q_snprintf( str, sizeof( str ), "fps:  435  ping: 533 ms lerp 112.3 ms   0/0" );
	g_pVGuiLocalize->ConvertANSIToUnicode( str, ustr, sizeof( ustr ) );
	int textTall;
	if ( m_hFontProportional == vgui::INVALID_FONT )
	{
		m_EstimatedWidth = textTall = 0;
	}
	else
	{
		g_pMatSystemSurface->GetTextSize( m_hFontProportional, ustr, m_EstimatedWidth, textTall );
	}

	int w, h;
	surface()->GetScreenSize( w, h );
	SetSize( w, h );
	SetPos( 0, 0 );

	ComputeNetgraphHeight();
}

void CNetGraphPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont( "DefaultFixedOutline", false );
	m_hFontProportional = pScheme->GetFont( "DefaultFixedOutline", true );
	m_hFontSmall = pScheme->GetFont( "DefaultVerySmall", false );

	OnFontChanged();
}

void CNetGraphPanel::ComputeNetgraphHeight()
{
	m_nNetGraphHeight = net_graphheight.GetInt();

	HFont fnt = GetNetgraphFont();
	int tall = surface()->GetFontTall( fnt );

	int lines = 3;
	if ( net_graph.GetInt() > 3 )
	{
		lines = 5;
	}
	else if ( net_graph.GetInt() > 2 )
	{
		lines = 4;
	}
	m_nNetGraphHeight = MAX( lines * tall, m_nNetGraphHeight );
}

//-----------------------------------------------------------------------------
// Purpose: Copies data from netcolor_t array into fields passed in
// Input  : color - 
//			*cv - 
//			*alpha - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GetColorValues( int color, byte *cv, byte *alpha )
{
	int i;
	netcolor_t *pc = &netcolors[ color ];
	for ( i = 0; i < 3; i++ )
	{
		cv[ i ] = pc->color[ i ];
	}
	*alpha = pc->alpha;
}

//-----------------------------------------------------------------------------
// Purpose: Sets appropriate color values
// Input  : *packet - 
//			*color - 
//			*ping - 
//			*alpha - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::ColorForHeight( packet_latency_t *packet, byte *color, int *ping, byte *alpha )
{
	int h = packet->latency;
	*ping = 0;
	switch ( h )
	{
	case 9999:
		GetColorValues( COLOR_DROPPED, color, alpha );
		break;
	case 9998:
		GetColorValues( COLOR_INVALID, color, alpha );
		break;
	case 9997:
		GetColorValues( COLOR_SKIPPED, color, alpha );
		break;
	default:
		*ping = 1;
		if (packet->choked )
		{
			GetColorValues( COLOR_CHOKED, color, alpha );
		}
		else
		{
			GetColorValues( COLOR_NORMAL, color, alpha );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set up blend colors for comman/client-frame/interpolation graph
//-----------------------------------------------------------------------------
void CNetGraphPanel::InitColors( void )
{
	int i, j;
	byte mincolor[2][3];
	byte maxcolor[2][3];
	float	dc[2][3];
	int		hfrac;	
	float	f;

	mincolor[0][0] = 63;
	mincolor[0][1] = 0;
	mincolor[0][2] = 100;

	maxcolor[0][0] = 0;
	maxcolor[0][1] = 63;
	maxcolor[0][2] = 255;

	mincolor[1][0] = 255;
	mincolor[1][1] = 127;
	mincolor[1][2] = 0;

	maxcolor[1][0] = 250;
	maxcolor[1][1] = 0;
	maxcolor[1][2] = 0;

	for ( i = 0; i < 3; i++ )
	{
		dc[0][i] = (float)(maxcolor[0][i] - mincolor[0][i]);
		dc[1][i] = (float)(maxcolor[1][i] - mincolor[1][i]);
	}

	hfrac = LERP_HEIGHT / 3;

	for ( i = 0; i < LERP_HEIGHT; i++ )
	{
		if ( i < hfrac )
		{
			f = (float)i / (float)hfrac;
			for ( j = 0; j < 3; j++ )
			{
				colors[ i ][j] = mincolor[0][j] + f * dc[0][j];
			}
		}
		else
		{
			f = (float)(i-hfrac) / (float)(LERP_HEIGHT - hfrac );
			for ( j = 0; j < 3; j++ )
			{
				colors[ i ][j] = mincolor[1][j] + f * dc[1][j];
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw client framerate / usercommand graph
// Input  : vrect - 
//			*cmdinfo - 
//			x - 
//			w - 
//-----------------------------------------------------------------------------

void CNetGraphPanel::DrawTimes( vrect_t vrect, cmdinfo_t *cmdinfo, int x, int w, int graphtype )
{
	if ( !net_graphshowinterp.GetBool() || graphtype <= 1 )
		return;

	int i;
	int j;
	int	extrap_point;
	int a, h;
	vrect_t  rcFill;

	ResetLineSegments();

	extrap_point = LERP_HEIGHT / 3;

	for (a=0 ; a<w ; a++)
	{
		i = ( m_OutgoingSequence - a ) & ( TIMINGS - 1 );
		h = MIN( ( cmdinfo[i].cmd_lerp / 3.0 ) * LERP_HEIGHT, LERP_HEIGHT );
		if ( h < 0 )
		{
			h = LERP_HEIGHT;
		}

		rcFill.x		= x + w -a - 1;
		rcFill.width	= 1;
		rcFill.height	= 1;

		rcFill.y = vrect.y + vrect.height - 4;
		
		if ( h >= extrap_point )
		{
			int start = 0;

			h -= extrap_point;
			rcFill.y -= extrap_point;

			if ( !net_graphsolid.GetInt() )
			{
				rcFill.y -= (h - 1);
				start = (h - 1);
			}

			for ( j = start; j < h; j++ )
			{
				int index = j + extrap_point;
				Assert( (size_t)index < Q_ARRAYSIZE( colors ) );
				DrawLine(&rcFill, colors[ index ], 255 );	
				rcFill.y--;
			}
		}
		else
		{
			int oldh;
			oldh = h;
			rcFill.y -= h;
			h = extrap_point - h;

			if ( !net_graphsolid.GetInt() )
			{
				h = 1;
			}

			for ( j = 0; j < h; j++ )
			{
				int index = j + oldh;
				Assert( (size_t)index < Q_ARRAYSIZE( colors ) );
				DrawLine(&rcFill, colors[ index ], 255 );	
				rcFill.y--;
			}
		}

		rcFill.y = vrect.y + vrect.height - 4 - extrap_point;

		DrawLine( &rcFill, extrap_base_color, 255 );

		rcFill.y = vrect.y + vrect.height - 3;

		if ( cmdinfo[ i ].sent )
		{
			DrawLine( &rcFill, sendcolor, 255 );
		}
		else
		{
			DrawLine( &rcFill, holdcolor, 200 );
		}
	}

	DrawLineSegments();
}

//-----------------------------------------------------------------------------
// Purpose: Compute frame database for rendering m_NetChannel computes choked, and lost packets, too.
//  Also computes latency data and sets max packet size
// Input  : *packet_latency - 
//			*graph - 
//			*choke_count - 
//			*loss_count - 
//			*biggest_message - 
//			1 - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GetFrameData( 	INetChannelInfo *netchannel, int *biggest_message, float *avg_message, float *f95thpercentile )
{
	float	frame_received_time;
	// float	frame_latency;

	*biggest_message	= 0;
	*avg_message		= 0.0f;
	*f95thpercentile	= 0.0f;

	int msg_count = 0;

	m_IncomingSequence = netchannel->GetSequenceNr( FLOW_INCOMING );
	m_OutgoingSequence = netchannel->GetSequenceNr( FLOW_OUTGOING );
	m_UpdateWindowSize = netchannel->GetBufferSize();
	m_AvgPacketLoss	   = netchannel->GetAvgLoss( FLOW_INCOMING );
	m_AvgPacketChoke   = netchannel->GetAvgChoke( FLOW_INCOMING );
	m_AvgLatency	   = netchannel->GetAvgLatency( FLOW_OUTGOING );
	m_IncomingData	   = netchannel->GetAvgData( FLOW_INCOMING ) / 1024.0f;
	m_OutgoingData	   = netchannel->GetAvgData( FLOW_OUTGOING ) / 1024.0f;
	m_AvgPacketIn      = netchannel->GetAvgPackets( FLOW_INCOMING );
	m_AvgPacketOut     = netchannel->GetAvgPackets( FLOW_OUTGOING );

	for ( int i=0; i<MAX_FLOWS; i++ )
		netchannel->GetStreamProgress( i, &m_StreamRecv[i], &m_StreamTotal[i] );

	float flAdjust = 0.0f;

	if ( cl_updaterate->GetFloat() > 0.001f )
	{
		flAdjust = -0.5f / cl_updaterate->GetFloat();

		m_AvgLatency += flAdjust;
	}

	// Can't be below zero
	m_AvgLatency = MAX( 0.0, m_AvgLatency );

	flAdjust *= 1000.0f;

	// Fill in frame data
	for ( int seqnr =m_IncomingSequence - m_UpdateWindowSize + 1
		; seqnr <= m_IncomingSequence
		; seqnr++)
	{
		
		
		frame_received_time = netchannel->GetPacketTime( FLOW_INCOMING, seqnr );

		netbandwidthgraph_t *nbwg = &m_Graph[ seqnr & ( TIMINGS - 1 )];
		packet_latency_t *lat = &m_PacketLatency[ seqnr & ( TIMINGS - 1 ) ];

		netchannel->GetPacketResponseLatency( FLOW_INCOMING, seqnr, &lat->latency, &lat->choked );

		if ( lat->latency < 9995 )
		{
			lat->latency += flAdjust;
			lat->latency = MAX( lat->latency, 0 );
		}		

		for ( int i=0; i<=INetChannelInfo::TOTAL; i++ )
		{
			nbwg->msgbytes[i] = netchannel->GetPacketBytes( FLOW_INCOMING, seqnr, i );
		}

		// Assert ( nbwg->msgbytes[INetChannelInfo::TOTAL] > 0 );

		if ( nbwg->msgbytes[INetChannelInfo::TOTAL] > *biggest_message )
		{
			*biggest_message = nbwg->msgbytes[INetChannelInfo::TOTAL];
		}

		*avg_message += (float)( nbwg->msgbytes[INetChannelInfo::TOTAL] );
		msg_count++;


	}

	if ( *biggest_message > 1000 )
	{
		*biggest_message = 1000;
	}

	if ( msg_count >= 1 )
	{
		*avg_message /= msg_count;

		int deviationsquared = 0;

		// Compute std deviation
		// Fill in frame data
		for (int seqnr=m_IncomingSequence - m_UpdateWindowSize + 1
			; seqnr <= m_IncomingSequence
			; seqnr++)
		{
			int bytes = m_Graph[ seqnr & ( TIMINGS - 1 )].msgbytes[INetChannelInfo::TOTAL] - ( *avg_message );

			deviationsquared += ( bytes * bytes );
		}

		float var = ( float )( deviationsquared ) / (float)( msg_count - 1 );
		float stddev = sqrt( var );

		*f95thpercentile = *avg_message + 2.0f * stddev;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fills in command interpolation/holdback & message size data
// Input  : *cmdinfo - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GetCommandInfo( INetChannelInfo *netchannel, cmdinfo_t *cmdinfo )
{
	for ( int seqnr = m_OutgoingSequence - m_UpdateWindowSize + 1
		; seqnr <= m_OutgoingSequence
		; seqnr++)
	{
		// Also set up the lerp point.
		cmdinfo_t *ci = &cmdinfo[ seqnr & ( TIMINGS - 1 ) ];

		ci->cmd_lerp = netchannel->GetCommandInterpolationAmount( FLOW_OUTGOING, seqnr );
		ci->sent =	netchannel->IsValidPacket( FLOW_OUTGOING, seqnr );
		ci->size =	netchannel->GetPacketBytes( FLOW_OUTGOING, seqnr, INetChannelInfo::TOTAL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws overlay text fields showing framerate, latency, bandwidth breakdowns, 
//  and, optionally, packet loss and choked packet percentages
// Input  : graphvalue - 
//			x - 
//			y - 
//			*graph - 
//			*cmdinfo - 
//			count - 
//			avg - 
//			*framerate - 
//			0.0 - 
//			avg - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawTextFields( int graphvalue, int x, int y, int w, netbandwidthgraph_t *graph, cmdinfo_t *cmdinfo )
{
	if ( !net_graphtext.GetBool() )
		return;

	static int lastout;

	char sz[ 256 ];
	int out;

	HFont font = GetNetgraphFont();

	// Move rolling average
	m_Framerate = FRAMERATE_AVG_FRAC * m_Framerate + ( 1.0 - FRAMERATE_AVG_FRAC ) * gpGlobals->absoluteframetime;

	// Print it out
	y -= m_nNetGraphHeight;

	int saveY = y;

	if ( m_Framerate <= 0.0f )
		m_Framerate = 1.0f;

	if ( engine->IsPlayingDemo() )
		m_AvgLatency = 0.0f;

	int textTall = surface()->GetFontTall( font );

	Q_snprintf( sz, sizeof( sz ), "fps:%4i   ping: %i ms", (int)(1.0f / m_Framerate), (int)(m_AvgLatency*1000.0f) );
	
	g_pMatSystemSurface->DrawColoredText( font, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%s", sz );

	// Draw update rate
	DrawUpdateRate( x + w, y );

	y += textTall;

	out = cmdinfo[ ( ( m_OutgoingSequence - 1 ) & ( TIMINGS - 1 ) ) ].size;
	if ( !out )
	{
		out = lastout;
	}
	else
	{
		lastout = out;
	}

	int totalsize = graph[ ( m_IncomingSequence & ( TIMINGS - 1 ) ) ].msgbytes[INetChannelInfo::TOTAL];
	
	Q_snprintf( sz, sizeof( sz ), "in :%4i   %2.2f k/s ", totalsize, m_IncomingData );

	int textWidth = g_pMatSystemSurface->DrawTextLen( font, "%s", sz );

	g_pMatSystemSurface->DrawColoredText( font, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%s", sz );

	Q_snprintf( sz, sizeof( sz ), "lerp: %5.1f ms", GetClientInterpAmount() * 1000.0f );

	int interpcolor[ 3 ] = { (int)GRAPH_RED, (int)GRAPH_GREEN, (int)GRAPH_BLUE }; 
	float flInterp = GetClientInterpAmount();
	if ( flInterp > 0.001f )
	{
		// Server framerate is lower than interp can possibly deal with
		if ( m_flServerFramerate < ( 1.0f / flInterp ) )
		{
			interpcolor[ 0 ] = 255;
			interpcolor[ 1 ] = 255;
			interpcolor[ 2 ] = 31;
		}
		// flInterp is below recommended setting!!!
		else if ( flInterp < ( 2.0f / cl_updaterate->GetFloat() ) )
		{
			interpcolor[ 0 ] = 255;
			interpcolor[ 1 ] = 125;
			interpcolor[ 2 ] = 31;
		}
	}

	g_pMatSystemSurface->DrawColoredText( font, x + textWidth, y, interpcolor[ 0 ], interpcolor[ 1 ], interpcolor[ 2 ], 255, "%s", sz );

	Q_snprintf( sz, sizeof( sz ), "%3.1f/s", m_AvgPacketIn );
	textWidth = g_pMatSystemSurface->DrawTextLen( font, "%s", sz );

	g_pMatSystemSurface->DrawColoredText( font, x + w - textWidth - 1, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%s", sz );

	y += textTall;

	Q_snprintf( sz, sizeof( sz ), "out:%4i   %2.2f k/s", out, m_OutgoingData );

	g_pMatSystemSurface->DrawColoredText( font, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%s", sz );

	Q_snprintf( sz, sizeof( sz ), "%3.1f/s", m_AvgPacketOut );
	textWidth = g_pMatSystemSurface->DrawTextLen( font, "%s", sz );

	g_pMatSystemSurface->DrawColoredText( font, x + w - textWidth - 1, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%s", sz );

	y += textTall;

	DrawCmdRate( x + w, y );

	if ( graphvalue > 2 )
	{
		Q_snprintf( sz, sizeof( sz ), "loss:%3i    choke: %2i ", (int)(m_AvgPacketLoss*100.0f), (int)(m_AvgPacketChoke*100.0f) );

		textWidth = g_pMatSystemSurface->DrawTextLen( font, "%s", sz );

		g_pMatSystemSurface->DrawColoredText( font, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%s", sz );

		y += textTall;

		if ( graphvalue > 3 )
		{
			Q_snprintf( sz, sizeof( sz ), "sv  : %5.1f   var: %4.2f msec", m_flServerFramerate, m_flServerFramerateStdDeviation * 1000.0f );

			int servercolor[ 3 ] = { (int)GRAPH_RED, (int)GRAPH_GREEN, (int)GRAPH_BLUE };

			if ( m_flServerFramerate < 10.0f )
			{
				servercolor[ 0 ] = 255;
				servercolor[ 1 ] = 31;
				servercolor[ 2 ] = 31;
			}
			else if ( m_flServerFramerate < 20.0f )
			{
				servercolor[ 0 ] = 255;
				servercolor[ 1 ] = 255;
				servercolor[ 2 ] = 0;
			}

			g_pMatSystemSurface->DrawColoredText( font, x, y, servercolor[ 0 ], servercolor[ 1 ], servercolor[ 2 ], 255, "%s", sz );

			y += textTall;
		}
	}

	// Draw legend
	if ( graphvalue >= 3 )
	{
		textTall = g_pMatSystemSurface->GetFontTall( m_hFontSmall );

		y = saveY - textTall - 5;
		int cw, ch;
		g_pMatSystemSurface->GetTextSize( m_hFontSmall, L"otherplayersWWW", cw, ch );
		if ( x - cw < 0 )
		{
			x += w + 5;
		}
		else
		{
			x -= cw;
		}

		g_pMatSystemSurface->DrawColoredText( m_hFontSmall, x, y, 0, 0, 255, 255, "localplayer" );
		y -= textTall;
		g_pMatSystemSurface->DrawColoredText( m_hFontSmall, x, y, 0, 255, 0, 255, "otherplayers" );
		y -= textTall;
		g_pMatSystemSurface->DrawColoredText( m_hFontSmall, x, y, 255, 0, 0, 255, "entities" );
		y -= textTall;
		g_pMatSystemSurface->DrawColoredText( m_hFontSmall, x, y, 255, 255, 0, 255, "sounds" );
		y -= textTall;
		g_pMatSystemSurface->DrawColoredText( m_hFontSmall, x, y, 0, 255, 255, 255, "events" );
		y -= textTall;
		g_pMatSystemSurface->DrawColoredText( m_hFontSmall, x, y, 128, 128, 0, 255, "usermessages" );
		y -= textTall;
		g_pMatSystemSurface->DrawColoredText( m_hFontSmall, x, y, 0, 128, 128, 255, "entmessages" );
		y -= textTall;
		g_pMatSystemSurface->DrawColoredText( m_hFontSmall, x, y, 128, 0, 0, 255, "stringcmds" );
		y -= textTall;
		g_pMatSystemSurface->DrawColoredText( m_hFontSmall, x, y, 0, 128, 0, 255, "stringtables" );
		y -= textTall;
		g_pMatSystemSurface->DrawColoredText( m_hFontSmall, x, y, 0, 0, 128, 255, "voice" );
		y -= textTall;
	}
	else
	{
		const CPUFrequencyResults frequency = GetCPUFrequencyResults();
		double currentTime = Plat_FloatTime();
		const double displayTime = 5.0f; // Display frequency results for this long.
		if ( frequency.m_GHz > 0 && frequency.m_timeStamp + displayTime > currentTime )
		{
			// Optionally print out the CPU frequency monitoring data.
			uint8 cpuColor[4] = { (uint8)GRAPH_RED, (uint8)GRAPH_GREEN, (uint8)GRAPH_BLUE, 255 };

			if ( frequency.m_percentage < kCPUMonitoringWarning2 )
			{
				cpuColor[0] = 255;
				cpuColor[1] = 31;
				cpuColor[2] = 31;
			}
			else if ( frequency.m_percentage < kCPUMonitoringWarning1 )
			{
				cpuColor[0] = 255;
				cpuColor[1] = 125;
				cpuColor[2] = 31;
			}
			// Experimental fading out as data becomes stale. Probably too distracting.
			//float age = currentTime - frequency.m_timeStamp;
			//cpuColor.a *= ( displayTime - age ) / displayTime;
			g_pMatSystemSurface->DrawColoredText( font, x, y, cpuColor[0], cpuColor[1], cpuColor[2], cpuColor[3],
						"CPU freq: %3.1f%%   Min: %3.1f%%", frequency.m_percentage, frequency.m_lowestPercentage );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determine type of graph to show, or if +graph key is held down, use detailed graph
// Output : int
//-----------------------------------------------------------------------------
int CNetGraphPanel::GraphValue( void )
{
	int graphtype;

	graphtype = net_graph.GetInt();
	
	if ( !graphtype && !( in_graph.state & 1 ) )
		return 0;

	// With +graph key, use max area
	if ( !graphtype )
	{
		graphtype = 2;
	}

	return graphtype;
}

//-----------------------------------------------------------------------------
// Purpose: Figure out x and y position for graph based on net_graphpos
//   value.
// Input  : *rect - 
//			width - 
//			*x - 
//			*y - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GraphGetXY( vrect_t *rect, int width, int *x, int *y )
{
	*x = rect->x + 5;

	switch ( net_graphpos.GetInt() )
	{
	case 0:
		break;
	case 1:
		*x = rect->x + rect->width - 5 - width;
		break;
	case 2:
		*x = rect->x + ( rect->width - 10 - width ) / 2;
		break;
	default:
		*x = rect->x + clamp( (int) XRES( net_graphpos.GetInt() ), 5, rect->width - width - 5 );
	}

	*y = rect->y+rect->height - LERP_HEIGHT - 5;
}

//-----------------------------------------------------------------------------
// Purpose: drawing stream progess (file download etc) as green bars ( under in/out)
// Input  : x - 
//			y - 
//			maxmsgbytes - 
//-----------------------------------------------------------------------------

void CNetGraphPanel::DrawStreamProgress( int x, int y, int width )
{
	vrect_t rcLine;

	rcLine.height	= 1;
	rcLine.x		= x;
	
	byte color[3]; color[0] = 0; color[1] = 200; color[2] = 0;

	if ( m_StreamTotal[FLOW_INCOMING] > 0 )
	{
		rcLine.y = y - m_nNetGraphHeight + 15 + 14;
		rcLine.width = (m_StreamRecv[FLOW_INCOMING]*width)/m_StreamTotal[FLOW_INCOMING];
		DrawLine( &rcLine, color, 255 );
	}

	if ( m_StreamTotal[FLOW_OUTGOING] > 0 )
	{
		rcLine.y = y - m_nNetGraphHeight + 2*15 + 14;
		rcLine.width = (m_StreamRecv[FLOW_OUTGOING]*width)/m_StreamTotal[FLOW_OUTGOING];
		DrawLine( &rcLine, color, 255 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: If showing bandwidth data, draw hatches big enough for largest message
// Input  : x - 
//			y - 
//			maxmsgbytes - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawHatches( int x, int y, int maxmsgbytes )
{
	int starty;
	int ystep;
	vrect_t rcHatch;

	byte colorminor[3];
	byte color[3];

	ystep = (int)( 10.0 / net_scale.GetFloat() );
	ystep = MAX( ystep, 1 );

	rcHatch.y		= y;
	rcHatch.height	= 1;
	rcHatch.x		= x;
	rcHatch.width	= 4;

	color[0] = 0;
	color[1] = 200;
	color[2] = 0;

	colorminor[0] = 63;
	colorminor[1] = 63;
	colorminor[2] = 0;

	for ( starty = rcHatch.y; rcHatch.y > 0 && ((starty - rcHatch.y)*net_scale.GetFloat() < ( maxmsgbytes + 50 ) ); rcHatch.y -= ystep )
	{
		if ( !((int)((starty - rcHatch.y)*net_scale.GetFloat() ) % 50 ) )
		{
			DrawLine( &rcHatch, color, 255 );
		}
		else if ( ystep > 5 )
		{
			DrawLine( &rcHatch, colorminor, 200 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: State how many updates a second are being requested
// Input  : x - 
//			y - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawUpdateRate( int xright, int y )
{
	char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i/s", cl_updaterate->GetInt() );
	wchar_t unicode[ 32 ];
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, unicode, sizeof( unicode  ) );

	// Last one
	int textWide, textTall;

	g_pMatSystemSurface->GetTextSize( GetNetgraphFont(), unicode, textWide, textTall );

	g_pMatSystemSurface->DrawColoredText( GetNetgraphFont(), xright - textWide - 1, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%s", sz );
}

//-----------------------------------------------------------------------------
// Purpose: State how many updates a second are being requested
// Input  : x - 
//			y - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawCmdRate( int xright, int y )
{
	char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i/s", cl_cmdrate->GetInt() );
	wchar_t unicode[ 32 ];
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, unicode, sizeof( unicode  ) );

	// Last one
	int textWide, textTall;

	g_pMatSystemSurface->GetTextSize( GetNetgraphFont(), unicode, textWide, textTall );

	g_pMatSystemSurface->DrawColoredText( GetNetgraphFont(), xright - textWide - 1, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%s", sz );
}

//-----------------------------------------------------------------------------
// Purpose: Draws bandwidth breakdown data
// Input  : *rcFill - 
//			bytes - 
//			r - 
//			g - 
//			b - 
//			alpha - 
// Output : int
//-----------------------------------------------------------------------------
int CNetGraphPanel::DrawDataSegment( vrect_t *rcFill, int bytes, byte r, byte g, byte b, byte alpha )
{
	int h;
	byte color[3];

	h = bytes / net_scale.GetFloat();
	
	color[0] = r;
	color[1] = g;
	color[2] = b;

	rcFill->height = h;
	rcFill->y -= h;

	if ( rcFill->y < 2 )
		return 0;

	DrawLine( rcFill, color, alpha );

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::OnTick( void )
{
	bool bVisible = ShouldDraw();
	if ( IsVisible() != bVisible )
	{
		SetVisible( bVisible );
	}
}

bool CNetGraphPanel::ShouldDraw( void )
{
	if ( GraphValue() != 0 )
		return true;

	return false;
}

void CNetGraphPanel::DrawLargePacketSizes( int x, int w, int graphtype, float warning_threshold )
{
	vrect_t		rcFill = {0,0,0,0};
	int a, i;

	for (a=0 ; a<w ; a++)
	{
		i = (m_IncomingSequence-a) & ( TIMINGS - 1 );
		
		rcFill.x			= x + w -a -1;
		rcFill.width		= 1;
		rcFill.y			= m_Graph[i].sampleY;
		rcFill.height		= m_Graph[i].sampleHeight;

		int nTotalBytes = m_Graph[ i ].msgbytes[ INetChannelInfo::TOTAL ];

		if ( warning_threshold != 0.0f &&
			nTotalBytes > MAX( 300, warning_threshold ) )
		{
			char sz[ 32 ];
			Q_snprintf( sz, sizeof( sz ), "%i", nTotalBytes );

			int len = g_pMatSystemSurface->DrawTextLen( m_hFont, "%s", sz );

			int textx, texty;

			textx = rcFill.x - len / 2;
			texty = MAX( 0, rcFill.y - 11 );

			g_pMatSystemSurface->DrawColoredText( m_hFont, textx, texty, 255, 255, 255, 255, "%s", sz );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A basic version (doesn't taken into account the "holding after
// screenshot" bit like TF does, but is good enough for hud_freezecamhide.
//-----------------------------------------------------------------------------
static bool IsTakingAFreezecamScreenshot()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	bool bInFreezeCam = ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );

	return ( bInFreezeCam && engine->IsTakingScreenshot() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::Paint() 
{
	VPROF( "CNetGraphPanel::Paint" );

	// Don't display net_graph if taking freezecam screenshot and hud_freezecamhide is enabled
	extern ConVar hud_freezecamhide;
	if ( hud_freezecamhide.GetBool() && IsTakingAFreezecamScreenshot() )
		return;

	int			graphtype;

	int			x, y;
	int			w;
	vrect_t		vrect;

	int			maxmsgbytes = 0;

	float		avg_message = 0.0f;
	float		warning_threshold = 0.0f;

	if ( ( graphtype = GraphValue() ) == 0 )
		return;

	// Since we divide by scale, make sure it's sensible
	if ( net_scale.GetFloat() <= 0 )
	{
		net_scale.SetValue( 0.1f );
	}

	int sw, sh;
	surface()->GetScreenSize( sw, sh );

	// Get screen rectangle
	vrect.x			= 0;
	vrect.y			= 0;
	vrect.width		= sw;
	vrect.height	= sh;


	w = MIN( (int)TIMINGS, m_EstimatedWidth );
	if ( vrect.width < w + 10 )
	{
		w = vrect.width - 10;
	}

	// get current client netchannel INetChannelInfo interface
	INetChannelInfo *nci = engine->GetNetChannelInfo();

	if ( nci )
	{
		// update incoming data
		GetFrameData( nci, &maxmsgbytes, &avg_message, &warning_threshold );

		// update outgoing data
		GetCommandInfo( nci, m_Cmdinfo );

		UpdateEstimatedServerFramerate( nci );
	}

	GraphGetXY( &vrect, w, &x, &y );

	if ( graphtype > 1 )
	{
		PaintLineArt( x, y, w, graphtype, maxmsgbytes );

		DrawLargePacketSizes( x, w, graphtype, warning_threshold );
	}

	// Draw client frame timing info
	DrawTimes( vrect, m_Cmdinfo, x, w, graphtype );

	DrawTextFields( graphtype, x, y, w, m_Graph, m_Cmdinfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::PaintLineArt( int x, int y, int w, int graphtype, int maxmsgbytes ) 
{
	VPROF( "CNetGraphPanel::PaintLineArt" );

	ResetLineSegments();

	int lastvalidh = 0;

	byte		color[3];
	int			ping;
	byte		alpha;
	vrect_t		rcFill = {0,0,0,0};

	int			pingheight = m_nNetGraphHeight - LERP_HEIGHT - 2;

	if (net_graphmsecs.GetInt() < 50 )
	{
		net_graphmsecs.SetValue( 50 );
	}

	bool bShowLatency = net_graphshowlatency.GetBool() && graphtype >= 2;

	for (int a=0 ; a<w ; a++)
	{
		int i = (m_IncomingSequence-a) & ( TIMINGS - 1 );
		int h = bShowLatency ? m_PacketLatency[i].latency : 0;
		
		packet_latency_t *pl = &m_PacketLatency[ i ];
		ColorForHeight( pl, color, &ping, &alpha );

		// Skipped
		if ( !ping ) 
		{
			// Re-use the last latency
			h = lastvalidh;  
		}
		else
		{
			h = pingheight * (float)h/net_graphmsecs.GetFloat();
			lastvalidh = h;
		}

		if ( h > pingheight )
		{
			h = pingheight;
		}

		rcFill.x		= x + w -a -1;
		rcFill.y		= y - h;
		rcFill.width	= 1;
		rcFill.height	= h;
		if ( ping )
		{
			rcFill.height	= pl->choked ? 2 : 1;
		}

		if ( !ping )
		{
			DrawLine2(&rcFill, color, color, alpha, 31 );		
		}
		else
		{
			DrawLine(&rcFill, color, alpha );		
		}

		rcFill.y		= y;
		rcFill.height	= 1;

		color[0] = 0;
		color[1] = 255;
		color[2] = 0;

		DrawLine( &rcFill, color, 160 );

		if ( graphtype < 2 )
			continue;

		// Draw a separator.
		rcFill.y = y - m_nNetGraphHeight - 1;
		rcFill.height = 1;

		color[0] = 255;
		color[1] = 255;
		color[2] = 255;

		DrawLine(&rcFill, color, 255 );		

		// Move up for begining of data
		rcFill.y -= 1;

		// Packet didn't have any real data...
		if ( m_PacketLatency[i].latency > 9995 )
			continue;


		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::LOCALPLAYER], 0, 0, 255 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::OTHERPLAYERS], 0, 255, 0 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::ENTITIES], 255, 0, 0 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::SOUNDS], 255, 255, 0) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::EVENTS], 0, 255, 255 ) )
			continue;
		
		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::USERMESSAGES], 128, 128, 0 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::ENTMESSAGES], 0, 128, 128 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::STRINGCMD], 128, 0, 0) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::STRINGTABLE], 0, 128, 0) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::VOICE], 0, 0, 128  ) )
			continue;

		// Final data chunk is total size, don't use solid line routine for this
		h = m_Graph[i].msgbytes[INetChannelInfo::TOTAL] / net_scale.GetFloat();

		color[ 0 ] = color[ 1 ] = color[ 2 ] = 240;

		rcFill.height = 1;
		rcFill.y = y - m_nNetGraphHeight - 1 - h;

		if ( rcFill.y < 2 )
			continue;

		DrawLine(&rcFill, color, 128 );		

		// Cache off height
		m_Graph[i].sampleY = rcFill.y;
		m_Graph[i].sampleHeight = rcFill.height;
	}

	if ( graphtype >= 2 )
	{
		// Draw hatches for first one:
		// on the far right side
		DrawHatches( x, y - m_nNetGraphHeight - 1, maxmsgbytes );
		
		DrawStreamProgress( x, y, w );
	}

	DrawLineSegments();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::ResetLineSegments()
{
	m_Rects.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawLineSegments()
{
	int c = m_Rects.Count();
	if ( c <= 0 )
		return;

	CMatRenderContextPtr pRenderContext( materials );
	IMesh* m_pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_WhiteMaterial );
	CMeshBuilder		meshBuilder;
	meshBuilder.Begin( m_pMesh, MATERIAL_LINES, c );

	int i;
	for ( i = 0 ; i < c; i++ )
	{
		CLineSegment *seg = &m_Rects[ i ];

		meshBuilder.Color4ubv( seg->color );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Position3f( seg->x1, seg->y1, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ubv( seg->color2 );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Position3f( seg->x2, seg->y2, 0 );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();

	m_pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: Draws a colored, filled rectangle
// Input  : *rect - 
//			*color - 
//			alpha - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawLine( vrect_t *rect, unsigned char *color, unsigned char alpha )
{
	DrawLine2( rect, color, color, alpha, alpha );
}

//-----------------------------------------------------------------------------
// Purpose: Draws a colored, filled rectangle
// Input  : *rect - 
//			*color - 
//			alpha - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawLine2( vrect_t *rect, unsigned char *color, unsigned char *color2, unsigned char alpha, unsigned char alpha2 )
{
	VPROF( "CNetGraphPanel::DrawLine2" );

	int idx = m_Rects.AddToTail();
	CLineSegment *seg = &m_Rects[ idx ];

	seg->color[0] = color[0];
	seg->color[1] = color[1];
	seg->color[2] = color[2];
	seg->color[3] = alpha;
	seg->color2[0] = color2[0];
	seg->color2[1] = color2[1];
	seg->color2[2] = color2[2];
	seg->color2[3] = alpha2;

	if ( rect->width == 1 )
	{
		seg->x1 = rect->x;
		seg->y1 = rect->y;
		seg->x2 = rect->x;
		seg->y2 = rect->y + rect->height;
	}
	else if ( rect->height == 1 )
	{
		seg->x1 = rect->x;
		seg->y1 = rect->y;
		seg->x2 = rect->x + rect->width;
		seg->y2 = rect->y;
	}
	else
	{
		Assert( 0 );
		m_Rects.Remove( idx );
	}
}

void CNetGraphPanel::UpdateEstimatedServerFramerate( INetChannelInfo *netchannel )
{
	float flFrameTime;
	netchannel->GetRemoteFramerate( &flFrameTime, &m_flServerFramerateStdDeviation );
	if ( flFrameTime > FLT_EPSILON )
	{
		m_flServerFramerate = 1.0f / flFrameTime;
	}
}

class CNetGraphPanelInterface : public INetGraphPanel
{
private:
	CNetGraphPanel *netGraphPanel;
public:
	CNetGraphPanelInterface( void )
	{
		netGraphPanel = NULL;
	}
	void Create( VPANEL parent )
	{
		netGraphPanel = new CNetGraphPanel( parent );
	}
	void Destroy( void )
	{
		if ( netGraphPanel )
		{
			netGraphPanel->SetParent( (Panel *)NULL );
			netGraphPanel->MarkForDeletion();
			netGraphPanel = NULL;
		}
	}
};

static CNetGraphPanelInterface g_NetGraphPanel;
INetGraphPanel *netgraphpanel = ( INetGraphPanel * )&g_NetGraphPanel;
