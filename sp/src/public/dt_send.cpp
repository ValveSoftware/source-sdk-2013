//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//


#include "dt_send.h"
#include "mathlib/mathlib.h"
#include "mathlib/vector.h"
#include "tier0/dbg.h"
#include "dt_utlvector_common.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if !defined(_STATIC_LINKED) || defined(GAME_DLL)


static CNonModifiedPointerProxy *s_pNonModifiedPointerProxyHead = NULL;


void SendProxy_UInt8ToInt32( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
void SendProxy_UInt16ToInt32( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
void SendProxy_UInt32ToInt32( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
#ifdef SUPPORTS_INT64
void SendProxy_UInt64ToInt64( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
#endif
const char *s_ElementNames[MAX_ARRAY_ELEMENTS] =
{
	"000", "001", "002", "003", "004", "005", "006", "007", "008", "009", 
	"010", "011", "012", "013", "014", "015", "016", "017", "018", "019",
	"020", "021", "022", "023", "024", "025", "026", "027", "028", "029",
	"030", "031", "032", "033", "034", "035", "036", "037", "038", "039",
	"040", "041", "042", "043", "044", "045", "046", "047", "048", "049",
	"050", "051", "052", "053", "054", "055", "056", "057", "058", "059",
	"060", "061", "062", "063", "064", "065", "066", "067", "068", "069",
	"070", "071", "072", "073", "074", "075", "076", "077", "078", "079",
	"080", "081", "082", "083", "084", "085", "086", "087", "088", "089",
	"090", "091", "092", "093", "094", "095", "096", "097", "098", "099",
	"100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
	"110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
	"120", "121", "122", "123", "124", "125", "126", "127", "128", "129",
	"130", "131", "132", "133", "134", "135", "136", "137", "138", "139",
	"140", "141", "142", "143", "144", "145", "146", "147", "148", "149",
	"150", "151", "152", "153", "154", "155", "156", "157", "158", "159",
	"160", "161", "162", "163", "164", "165", "166", "167", "168", "169",
	"170", "171", "172", "173", "174", "175", "176", "177", "178", "179",
	"180", "181", "182", "183", "184", "185", "186", "187", "188", "189",
	"190", "191", "192", "193", "194", "195", "196", "197", "198", "199",
	"200", "201", "202", "203", "204", "205", "206", "207", "208", "209",
	"210", "211", "212", "213", "214", "215", "216", "217", "218", "219",
	"220", "221", "222", "223", "224", "225", "226", "227", "228", "229",
	"230", "231", "232", "233", "234", "235", "236", "237", "238", "239",
	"240", "241", "242", "243", "244", "245", "246", "247", "248", "249",
	"250", "251", "252", "253", "254", "255", "256", "257", "258", "259",
	"260", "261", "262", "263", "264", "265", "266", "267", "268", "269",
	"270", "271", "272", "273", "274", "275", "276", "277", "278", "279",
	"280", "281", "282", "283", "284", "285", "286", "287", "288", "289",
	"290", "291", "292", "293", "294", "295", "296", "297", "298", "299",
	"300", "301", "302", "303", "304", "305", "306", "307", "308", "309",
	"310", "311", "312", "313", "314", "315", "316", "317", "318", "319",
	"320", "321", "322", "323", "324", "325", "326", "327", "328", "329",
	"330", "331", "332", "333", "334", "335", "336", "337", "338", "339",
	"340", "341", "342", "343", "344", "345", "346", "347", "348", "349",
	"350", "351", "352", "353", "354", "355", "356", "357", "358", "359",
	"360", "361", "362", "363", "364", "365", "366", "367", "368", "369",
	"370", "371", "372", "373", "374", "375", "376", "377", "378", "379",
	"380", "381", "382", "383", "384", "385", "386", "387", "388", "389",
	"390", "391", "392", "393", "394", "395", "396", "397", "398", "399",
	"400", "401", "402", "403", "404", "405", "406", "407", "408", "409",
	"410", "411", "412", "413", "414", "415", "416", "417", "418", "419",
	"420", "421", "422", "423", "424", "425", "426", "427", "428", "429",
	"430", "431", "432", "433", "434", "435", "436", "437", "438", "439",
	"440", "441", "442", "443", "444", "445", "446", "447", "448", "449",
	"450", "451", "452", "453", "454", "455", "456", "457", "458", "459",
	"460", "461", "462", "463", "464", "465", "466", "467", "468", "469",
	"470", "471", "472", "473", "474", "475", "476", "477", "478", "479",
	"480", "481", "482", "483", "484", "485", "486", "487", "488", "489",
	"490", "491", "492", "493", "494", "495", "496", "497", "498", "499",
	"500", "501", "502", "503", "504", "505", "506", "507", "508", "509",
	"510", "511", "512", "513", "514", "515", "516", "517", "518", "519",
	"520", "521", "522", "523", "524", "525", "526", "527", "528", "529",
	"530", "531", "532", "533", "534", "535", "536", "537", "538", "539",
	"540", "541", "542", "543", "544", "545", "546", "547", "548", "549",
	"550", "551", "552", "553", "554", "555", "556", "557", "558", "559",
	"560", "561", "562", "563", "564", "565", "566", "567", "568", "569",
	"570", "571", "572", "573", "574", "575", "576", "577", "578", "579",
	"580", "581", "582", "583", "584", "585", "586", "587", "588", "589",
	"590", "591", "592", "593", "594", "595", "596", "597", "598", "599",
	"600", "601", "602", "603", "604", "605", "606", "607", "608", "609",
	"610", "611", "612", "613", "614", "615", "616", "617", "618", "619",
	"620", "621", "622", "623", "624", "625", "626", "627", "628", "629",
	"630", "631", "632", "633", "634", "635", "636", "637", "638", "639",
	"640", "641", "642", "643", "644", "645", "646", "647", "648", "649",
	"650", "651", "652", "653", "654", "655", "656", "657", "658", "659",
	"660", "661", "662", "663", "664", "665", "666", "667", "668", "669",
	"670", "671", "672", "673", "674", "675", "676", "677", "678", "679",
	"680", "681", "682", "683", "684", "685", "686", "687", "688", "689",
	"690", "691", "692", "693", "694", "695", "696", "697", "698", "699",
	"700", "701", "702", "703", "704", "705", "706", "707", "708", "709",
	"710", "711", "712", "713", "714", "715", "716", "717", "718", "719",
	"720", "721", "722", "723", "724", "725", "726", "727", "728", "729",
	"730", "731", "732", "733", "734", "735", "736", "737", "738", "739",
	"740", "741", "742", "743", "744", "745", "746", "747", "748", "749",
	"750", "751", "752", "753", "754", "755", "756", "757", "758", "759",
	"760", "761", "762", "763", "764", "765", "766", "767", "768", "769",
	"770", "771", "772", "773", "774", "775", "776", "777", "778", "779",
	"780", "781", "782", "783", "784", "785", "786", "787", "788", "789",
	"790", "791", "792", "793", "794", "795", "796", "797", "798", "799",
	"800", "801", "802", "803", "804", "805", "806", "807", "808", "809",
	"810", "811", "812", "813", "814", "815", "816", "817", "818", "819",
	"820", "821", "822", "823", "824", "825", "826", "827", "828", "829",
	"830", "831", "832", "833", "834", "835", "836", "837", "838", "839",
	"840", "841", "842", "843", "844", "845", "846", "847", "848", "849",
	"850", "851", "852", "853", "854", "855", "856", "857", "858", "859",
	"860", "861", "862", "863", "864", "865", "866", "867", "868", "869",
	"870", "871", "872", "873", "874", "875", "876", "877", "878", "879",
	"880", "881", "882", "883", "884", "885", "886", "887", "888", "889",
	"890", "891", "892", "893", "894", "895", "896", "897", "898", "899",
	"900", "901", "902", "903", "904", "905", "906", "907", "908", "909",
	"910", "911", "912", "913", "914", "915", "916", "917", "918", "919",
	"920", "921", "922", "923", "924", "925", "926", "927", "928", "929",
	"930", "931", "932", "933", "934", "935", "936", "937", "938", "939",
	"940", "941", "942", "943", "944", "945", "946", "947", "948", "949",
	"950", "951", "952", "953", "954", "955", "956", "957", "958", "959",
	"960", "961", "962", "963", "964", "965", "966", "967", "968", "969",
	"970", "971", "972", "973", "974", "975", "976", "977", "978", "979",
	"980", "981", "982", "983", "984", "985", "986", "987", "988", "989",
	"990", "991", "992", "993", "994", "995", "996", "997", "998", "999",
	"1000", "1001", "1002", "1003", "1004", "1005", "1006", "1007", "1008", "1009",
	"1010", "1011", "1012", "1013", "1014", "1015", "1016", "1017", "1018", "1019",
	"1020", "1021", "1022", "1023"

};


CNonModifiedPointerProxy::CNonModifiedPointerProxy( SendTableProxyFn fn )
{
	m_pNext = s_pNonModifiedPointerProxyHead;
	s_pNonModifiedPointerProxyHead = this;
	m_Fn = fn;
}


CStandardSendProxiesV1::CStandardSendProxiesV1()
{
	m_Int8ToInt32 = SendProxy_Int8ToInt32;
	m_Int16ToInt32 = SendProxy_Int16ToInt32;
	m_Int32ToInt32 = SendProxy_Int32ToInt32;
#ifdef SUPPORTS_INT64
	m_Int64ToInt64 = SendProxy_Int64ToInt64;
#endif

	m_UInt8ToInt32 = SendProxy_UInt8ToInt32;
	m_UInt16ToInt32 = SendProxy_UInt16ToInt32;
	m_UInt32ToInt32 = SendProxy_UInt32ToInt32;
#ifdef SUPPORTS_INT64
	m_UInt64ToInt64 = SendProxy_UInt64ToInt64;
#endif
	
	m_FloatToFloat = SendProxy_FloatToFloat;
	m_VectorToVector = SendProxy_VectorToVector;
}

CStandardSendProxies::CStandardSendProxies()
{	
	m_DataTableToDataTable = SendProxy_DataTableToDataTable;
	m_SendLocalDataTable = SendProxy_SendLocalDataTable;
	m_ppNonModifiedPointerProxies = &s_pNonModifiedPointerProxyHead;
	
}
CStandardSendProxies g_StandardSendProxies;


// ---------------------------------------------------------------------- //
// Proxies.
// ---------------------------------------------------------------------- //
void SendProxy_AngleToFloat( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	float angle;

	angle = *((float*)pData);
	pOut->m_Float = anglemod( angle );

	Assert( IsFinite( pOut->m_Float ) );
}

void SendProxy_FloatToFloat( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Float = *((float*)pData);
	Assert( IsFinite( pOut->m_Float ) );
}

void SendProxy_QAngles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	QAngle *v = (QAngle*)pData;
	pOut->m_Vector[0] = anglemod( v->x );
	pOut->m_Vector[1] = anglemod( v->y );
	pOut->m_Vector[2] = anglemod( v->z );
}

void SendProxy_VectorToVector( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	Vector& v = *(Vector*)pData;
	Assert( v.IsValid() );
	pOut->m_Vector[0] = v[0];
	pOut->m_Vector[1] = v[1];
	pOut->m_Vector[2] = v[2];
}

void SendProxy_VectorXYToVectorXY( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	Vector& v = *(Vector*)pData;
	Assert( v.IsValid() );
	pOut->m_Vector[0] = v[0];
	pOut->m_Vector[1] = v[1];
}

#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
void SendProxy_QuaternionToQuaternion( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	Quaternion& q = *(Quaternion*)pData;
	Assert( q.IsValid() );
	pOut->m_Vector[0] = q[0];
	pOut->m_Vector[1] = q[1];
	pOut->m_Vector[2] = q[2];
	pOut->m_Vector[3] = q[3];
}
#endif

void SendProxy_Int8ToInt32( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((const char*)pData);
}

void SendProxy_Int16ToInt32( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((short*)pData);
}

void SendProxy_Int32ToInt32( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((int*)pData);
}

#ifdef SUPPORTS_INT64
void SendProxy_Int64ToInt64( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int64 = *((int64*)pData);
}
#endif

void SendProxy_UInt8ToInt32( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((const unsigned char*)pData);
}

void SendProxy_UInt16ToInt32( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((unsigned short*)pData);
}

void SendProxy_UInt32ToInt32( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	*((unsigned long*)&pOut->m_Int) = *((unsigned long*)pData);
}
#ifdef SUPPORTS_INT64
void SendProxy_UInt64ToInt64( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	*((int64*)&pOut->m_Int64) = *((uint64*)pData);
}
#endif

void SendProxy_StringToString( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_pString = (const char*)pData;
}

void* SendProxy_DataTableToDataTable( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	return (void*)pData;
}

void* SendProxy_DataTablePtrToDataTable( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	return *((void**)pData);
}

static void SendProxy_Empty( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
}

//-----------------------------------------------------------------------------
// Purpose: If the recipient is the same as objectID, go ahead and iterate down
//  the m_Local stuff, otherwise, act like it wasn't there at all.
// This way, only the local player receives information about him/herself.
// Input  : *pVarData - 
//			*pOut - 
//			objectID - 
//-----------------------------------------------------------------------------

void* SendProxy_SendLocalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	pRecipients->SetOnly( objectID - 1 );
	return ( void * )pVarData;
}





// ---------------------------------------------------------------------- //
// Prop setup functions (for building tables).
// ---------------------------------------------------------------------- //
float AssignRangeMultiplier( int nBits, double range )
{
	unsigned long iHighValue;
	if ( nBits == 32 )
		iHighValue = 0xFFFFFFFE;
	else
		iHighValue = ((1 << (unsigned long)nBits) - 1);

	float fHighLowMul = iHighValue / range;
	if ( CloseEnough( range, 0 ) )
		fHighLowMul = iHighValue;
	
	// If the precision is messing us up, then adjust it so it won't.
	if ( (unsigned long)(fHighLowMul * range) > iHighValue ||
		 (fHighLowMul * range) > (double)iHighValue )
	{
		// Squeeze it down smaller and smaller until it's going to produce an integer
		// in the valid range when given the highest value.
		float multipliers[] = { 0.9999, 0.99, 0.9, 0.8, 0.7 };
		int i;
		for ( i=0; i < ARRAYSIZE( multipliers ); i++ )
		{
			fHighLowMul = (float)( iHighValue / range ) * multipliers[i];
			if ( (unsigned long)(fHighLowMul * range) > iHighValue ||
				(fHighLowMul * range) > (double)iHighValue )
			{
			}
			else
			{
				break;
			}
		}

		if ( i == ARRAYSIZE( multipliers ) )
		{
			// Doh! We seem to be unable to represent this range.
			Assert( false );
			return 0;
		}
	}

	return fHighLowMul;
}



SendProp SendPropFloat(
	const char *pVarName,		
	// Variable name.
	int offset,			// Offset into container structure.
	int sizeofVar,
	int nBits,			// Number of bits to use when encoding.
	int flags,
	float fLowValue,		// For floating point, low and high values.
	float fHighValue,		// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if ( varProxy == SendProxy_FloatToFloat )
	{
		Assert( sizeofVar == 0 || sizeofVar == 4 );
	}

	if ( nBits <= 0 || nBits == 32 )
	{
		flags |= SPROP_NOSCALE;
		fLowValue = 0.f;
		fHighValue = 0.f;
	}
	else
	{
		if(fHighValue == HIGH_DEFAULT)
			fHighValue = (1 << nBits);

		if (flags & SPROP_ROUNDDOWN)
			fHighValue = fHighValue - ((fHighValue - fLowValue) / (1 << nBits));
		else if (flags & SPROP_ROUNDUP)
			fLowValue = fLowValue + ((fHighValue - fLowValue) / (1 << nBits));
	}

	ret.m_Type = DPT_Float;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = fLowValue;
	ret.m_fHighValue = fHighValue;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );
	if( ret.GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL | SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL ) )
		ret.m_nBits = 0;

	return ret;
}

SendProp SendPropVector(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,					// Number of bits to use when encoding.
	int flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue,			// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if(varProxy == SendProxy_VectorToVector)
	{
		Assert(sizeofVar == sizeof(Vector));
	}

	if ( nBits == 32 )
		flags |= SPROP_NOSCALE;

	ret.m_Type = DPT_Vector;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = fLowValue;
	ret.m_fHighValue = fHighValue;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );
	if( ret.GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL | SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL) )
		ret.m_nBits = 0;

	return ret;
}

SendProp SendPropVectorXY(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,					// Number of bits to use when encoding.
	int flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue,			// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if(varProxy == SendProxy_VectorXYToVectorXY)
	{
		Assert(sizeofVar == sizeof(Vector));
	}

	if ( nBits == 32 )
		flags |= SPROP_NOSCALE;

	ret.m_Type = DPT_VectorXY;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = fLowValue;
	ret.m_fHighValue = fHighValue;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );
	if( ret.GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL | SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL) )
		ret.m_nBits = 0;

	return ret;
}

#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
SendProp SendPropQuaternion(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,					// Number of bits to use when encoding.
	int flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue,			// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if(varProxy == SendProxy_QuaternionToQuaternion)
	{
		Assert(sizeofVar == sizeof(Quaternion));
	}

	if ( nBits == 32 )
		flags |= SPROP_NOSCALE;

	ret.m_Type = DPT_Quaternion;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = fLowValue;
	ret.m_fHighValue = fHighValue;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );
	if( ret.GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL | SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL) )
		ret.m_nBits = 0;

	return ret;
}
#endif

SendProp SendPropAngle(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	int flags,
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if(varProxy == SendProxy_AngleToFloat)
	{
		Assert(sizeofVar == 4);
	}

	if ( nBits == 32 )
		flags |= SPROP_NOSCALE;

	ret.m_Type = DPT_Float;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = 0.0f;
	ret.m_fHighValue = 360.0f;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );

	return ret;
}


SendProp SendPropQAngles(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	int flags,
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if(varProxy == SendProxy_AngleToFloat)
	{
		Assert(sizeofVar == 4);
	}

	if ( nBits == 32 )
		flags |= SPROP_NOSCALE;

	ret.m_Type = DPT_Vector;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = 0.0f;
	ret.m_fHighValue = 360.0f;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );

	ret.SetProxyFn( varProxy );

	return ret;
}
  
SendProp SendPropInt(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	int flags,
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if ( !varProxy )
	{
		if ( sizeofVar == 1 )
		{
			varProxy = SendProxy_Int8ToInt32;
		}
		else if ( sizeofVar == 2 )
		{
			varProxy = SendProxy_Int16ToInt32;
		}
		else if ( sizeofVar == 4 )
		{
			varProxy = SendProxy_Int32ToInt32;
		}
#ifdef SUPPORTS_INT64
		else if ( sizeofVar == 8 )
		{
			varProxy = SendProxy_Int64ToInt64;
		}
#endif
		else
		{
			Assert(!"SendPropInt var has invalid size");
			varProxy = SendProxy_Int8ToInt32;	// safest one...
		}
	}

	// Figure out # of bits if the want us to.
	if ( nBits <= 0 )
	{
		Assert( sizeofVar == 1 || sizeofVar == 2 || sizeofVar == 4 );
		nBits = sizeofVar * 8;
	}

#ifdef SUPPORTS_INT64
	ret.m_Type = (sizeofVar == 8) ? DPT_Int64 : DPT_Int;
#else
	ret.m_Type = DPT_Int;
#endif
	
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );

	// Use UInt proxies if they want unsigned data. This isn't necessary to encode
	// the values correctly, but it lets us check the ranges of the data to make sure
	// they're valid.
	ret.SetProxyFn( varProxy );
	if( ret.GetFlags() & SPROP_UNSIGNED )
	{
		if( varProxy == SendProxy_Int8ToInt32 )
			ret.SetProxyFn( SendProxy_UInt8ToInt32 );
		
		else if( varProxy == SendProxy_Int16ToInt32 )
			ret.SetProxyFn( SendProxy_UInt16ToInt32 );

		else if( varProxy == SendProxy_Int32ToInt32 )
			ret.SetProxyFn( SendProxy_UInt32ToInt32 );
			
#ifdef SUPPORTS_INT64
		else if( varProxy == SendProxy_Int64ToInt64 )
			ret.SetProxyFn( SendProxy_UInt64ToInt64 );
#endif
	}

	return ret;
}

SendProp SendPropString(
	const char *pVarName,
	int offset,
	int bufferLen,
	int flags,
	SendVarProxyFn varProxy)
{
	SendProp ret;

	Assert( bufferLen <= DT_MAX_STRING_BUFFERSIZE ); // You can only have strings with 8-bits worth of length.
	
	ret.m_Type = DPT_String;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.SetFlags( flags );
	ret.SetProxyFn( varProxy );

	return ret;
}

SendProp SendPropArray3(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int elements,
	SendProp pArrayProp,
	SendTableProxyFn varProxy
	)
{
	SendProp ret;

	Assert( elements <= MAX_ARRAY_ELEMENTS );

	ret.m_Type = DPT_DataTable;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.SetDataTableProxyFn( varProxy );
	
	SendProp *pArrayPropAllocated = new SendProp;
	*pArrayPropAllocated = pArrayProp;
	ret.SetArrayProp( pArrayPropAllocated );
	
	// Handle special proxy types where they always let all clients get the results.
	if ( varProxy == SendProxy_DataTableToDataTable || varProxy == SendProxy_DataTablePtrToDataTable )
	{
		ret.SetFlags( SPROP_PROXY_ALWAYS_YES );
	}

	SendProp *pProps = new SendProp[elements]; // TODO free that again
	
	for ( int i = 0; i < elements; i++ )
	{
		pProps[i] = pArrayProp;	// copy array element property setting
		pProps[i].SetOffset( i*sizeofVar ); // adjust offset
		pProps[i].m_pVarName = s_ElementNames[i];	// give unique name
		pProps[i].m_pParentArrayPropName = pVarName; // For debugging...
	}

	SendTable *pTable = new SendTable( pProps, elements, pVarName ); // TODO free that again

	ret.SetDataTable( pTable );

	return ret;
}

SendProp SendPropDataTable(
	const char *pVarName,
	int offset,
	SendTable *pTable,
	SendTableProxyFn varProxy
	)
{
	SendProp ret;

	ret.m_Type = DPT_DataTable;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.SetDataTable( pTable );
	ret.SetDataTableProxyFn( varProxy );
	
	// Handle special proxy types where they always let all clients get the results.
	if ( varProxy == SendProxy_DataTableToDataTable || varProxy == SendProxy_DataTablePtrToDataTable )
	{
		ret.SetFlags( SPROP_PROXY_ALWAYS_YES );
	}
	
	if ( varProxy == SendProxy_DataTableToDataTable && offset == 0 )
	{
		ret.SetFlags( SPROP_COLLAPSIBLE );
	}

	return ret;
}


SendProp InternalSendPropArray(
	const int elementCount,
	const int elementStride,
	const char *pName,
	ArrayLengthSendProxyFn arrayLengthFn
	)
{
	SendProp ret;

	ret.m_Type = DPT_Array;
	ret.m_nElements = elementCount;
	ret.m_ElementStride = elementStride;
	ret.m_pVarName = pName;
	ret.SetProxyFn( SendProxy_Empty );
	ret.m_pArrayProp = NULL;	// This gets set in SendTable_InitTable. It always points at the property that precedes
								// this one in the datatable's list.
	ret.SetArrayLengthProxy( arrayLengthFn );
		
	return ret;
}


SendProp SendPropExclude(
	const char *pDataTableName,	// Data table name (given to BEGIN_SEND_TABLE and BEGIN_RECV_TABLE).
	const char *pPropName		// Name of the property to exclude.
	)
{
	SendProp ret;

	ret.SetFlags( SPROP_EXCLUDE );
	ret.m_pExcludeDTName = pDataTableName;
	ret.m_pVarName = pPropName;

	return ret;
}



// ---------------------------------------------------------------------- //
// SendProp
// ---------------------------------------------------------------------- //
SendProp::SendProp()
{
	m_pVarName = NULL;
	m_Offset = 0;
	m_pDataTable = NULL;
	m_ProxyFn = NULL;
	m_pExcludeDTName = NULL;
	m_pParentArrayPropName = NULL;

	
	m_Type = DPT_Int;
	m_Flags = 0;
	m_nBits = 0;

	m_fLowValue = 0.0f;
	m_fHighValue = 0.0f;
	m_pArrayProp = 0;
	m_ArrayLengthProxy = 0;
	m_nElements = 1;
	m_ElementStride = -1;
}


SendProp::~SendProp()
{
}


int SendProp::GetNumArrayLengthBits() const
{
	Assert( GetType() == DPT_Array );
#if _X360
	int elemCount = GetNumElements();
	if ( !elemCount )
		return 1;
	return (32 - _CountLeadingZeros(GetNumElements()));
#else
	return Q_log2( GetNumElements() ) + 1;
#endif
}


// ---------------------------------------------------------------------- //
// SendTable
// ---------------------------------------------------------------------- //
SendTable::SendTable()
{
	Construct( NULL, 0, NULL );
}


SendTable::SendTable(SendProp *pProps, int nProps, const char *pNetTableName)
{
	Construct( pProps, nProps, pNetTableName );
}


SendTable::~SendTable()
{
//	Assert( !m_pPrecalc );
}


void SendTable::Construct( SendProp *pProps, int nProps, const char *pNetTableName )
{
	m_pProps = pProps;
	m_nProps = nProps;
	m_pNetTableName = pNetTableName;
	m_pPrecalc = 0;
	m_bInitialized = false;
	m_bHasBeenWritten = false;
	m_bHasPropsEncodedAgainstCurrentTickCount = false;
}

#endif
