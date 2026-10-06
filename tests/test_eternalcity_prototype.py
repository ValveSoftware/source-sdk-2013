import unittest
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]


class EternalCityPrototypeContractTests(unittest.TestCase):
    def test_mod_targets_source_sdk_base_singleplayer(self):
        gameinfo = (ROOT / "game/mod_hl2/gameinfo.txt").read_text()
        self.assertRegex(gameinfo, r"SteamAppId\s+243730")
        self.assertIsNotNone(
            re.search(r'^\s*game\s+"Eternal City Prototype"\s*$', gameinfo, re.MULTILINE)
        )

    def test_hud_source_is_in_hl2_client_project(self):
        project = (ROOT / "src/game/client/client_hl2.vpc").read_text()
        self.assertIn('$File\t"hl2\\eternalcity_hud.cpp"', project)

    def test_hud_reads_live_player_health_and_weapon_ammo(self):
        hud = (ROOT / "src/game/client/hl2/eternalcity_hud.cpp").read_text()
        self.assertIn("C_BasePlayer::GetLocalPlayer()", hud)
        self.assertIn("pPlayer->Health()", hud)
        self.assertIn("pWeapon->Clip1()", hud)
        self.assertIn("pPlayer->GetAmmoCount(", hud)

    def test_hud_respects_source_hud_visibility(self):
        header = (ROOT / "src/game/client/hl2/eternalcity_hud.h").read_text()
        hud = (ROOT / "src/game/client/hl2/eternalcity_hud.cpp").read_text()
        self.assertIn('#include "hudelement.h"', header)
        self.assertIn("class CEternalCityHud : public CHudElement, public vgui::Panel", header)
        self.assertIn("DECLARE_HUDELEMENT( CEternalCityHud )", hud)
        self.assertIn("return CHudElement::ShouldDraw()", hud)
        self.assertIn("if ( !ShouldDraw() )", hud)
        public_members = header.split("public:", 1)[1].split("protected:", 1)[0]
        self.assertIn("DECLARE_MULTIPLY_INHERITED();", public_members)
        client_mode = (ROOT / "src/game/client/hl2/clientmode_hlnormal.cpp").read_text()
        self.assertIn("GET_HUDELEMENT( CEternalCityHud )", client_mode)
        self.assertNotIn("new CEternalCityHud", client_mode)

    def test_modal_panels_have_mock_actions_and_modal_input(self):
        hud = (ROOT / "src/game/client/hl2/eternalcity_hud.cpp").read_text()
        self.assertIn("SetAppModalSurface(", hud)
        self.assertIn("ReleaseAppModalSurface()", hud)
        self.assertIn("ToggleEquipment()", hud)
        self.assertIn("ToggleInventory()", hud)
        self.assertIn("m_bPrototypeVestEquipped", hud)
        self.assertIn("m_iMockMedkits", hud)
        client_mode = (ROOT / "src/game/client/hl2/clientmode_hlnormal.cpp").read_text()
        self.assertIn("cmd->buttons = 0", client_mode)
        self.assertIn("if ( !down )", client_mode)

    def test_mock_actions_only_fire_inside_the_drawn_button(self):
        hud = (ROOT / "src/game/client/hl2/eternalcity_hud.cpp").read_text()
        handler = hud.split("void CEternalCityHud::OnMousePressed", 1)[1]
        self.assertIn("const int buttonWidth = MIN( 250, panelWidth - 56 );", handler)
        self.assertIn("cursorX < contentX || cursorX >= contentX + buttonWidth", handler)
        self.assertIn("cursorY < buttonY || cursorY >= buttonY + 38", handler)

    def test_status_shortcut_preserves_default_use_and_escape_closes(self):
        client_mode = (ROOT / "src/game/client/hl2/clientmode_hlnormal.cpp").read_text()
        self.assertIn("KEY_E", client_mode)
        self.assertIn("KEY_LCONTROL", client_mode)
        self.assertIn("KEY_RCONTROL", client_mode)
        self.assertIn("KEY_ESCAPE", client_mode)
        self.assertIn("BaseClass::KeyInput", client_mode)

    def test_follow_camera_traces_walls_and_skips_non_player_views(self):
        client_mode = (ROOT / "src/game/client/hl2/clientmode_hlnormal.cpp").read_text()
        self.assertIn("ClientModeHLNormal::OverrideView", client_mode)
        self.assertIn("UTIL_TraceHull(", client_mode)
        self.assertIn("IsInAVehicle()", client_mode)
        self.assertIn("GetObserverMode()", client_mode)

    def test_camera_includes_complete_view_definition(self):
        client_mode = (ROOT / "src/game/client/hl2/clientmode_hlnormal.cpp").read_text()
        self.assertIn('#include "view_shared.h"', client_mode)

    def test_follow_camera_connects_source_third_person_rendering(self):
        client_mode = (ROOT / "src/game/client/hl2/clientmode_hlnormal.cpp").read_text()
        self.assertIn("input->CAM_ToThirdPerson();", client_mode)
        self.assertIn("pPlayer->ThirdPersonSwitch( true );", client_mode)
        self.assertIn("if ( m_bOwnsThirdPerson )", client_mode)
        self.assertIn("input->CAM_ToFirstPerson();", client_mode)
        self.assertRegex(client_mode, r"void ClientModeHLNormal::LevelShutdown\( void \)\s*\{\s*ReleaseEternalCityCamera\(\);")

    def test_modal_closes_on_level_shutdown(self):
        header = (ROOT / "src/game/client/hl2/eternalcity_hud.h").read_text()
        hud = (ROOT / "src/game/client/hl2/eternalcity_hud.cpp").read_text()
        self.assertIn("virtual void LevelShutdown();", header)
        self.assertRegex(hud, r"void CEternalCityHud::LevelShutdown\(\)\s*\{\s*CloseModal\(\);")

    def test_modal_suppresses_impulses_and_weapon_selection(self):
        client_mode = (ROOT / "src/game/client/hl2/clientmode_hlnormal.cpp").read_text()
        modal_move = client_mode.split("bool ClientModeHLNormal::CreateMove", 1)[1]
        modal_move = modal_move.split("return BaseClass::CreateMove", 1)[0]
        for field in ("impulse", "weaponselect", "weaponsubtype"):
            self.assertIn("cmd->%s = 0;" % field, modal_move)

    def test_build_notes_separate_build_from_install_and_mark_blocker(self):
        guide_path = ROOT / "docs/ETERNALCITY_PROTOTYPE.md"
        self.assertTrue(guide_path.exists(), "prototype build guide is missing")
        guide = guide_path.read_text()
        self.assertIn("243730", guide)
        self.assertIn("singleplayer", guide.lower())
        self.assertIn("smoke test", guide.lower())
        self.assertIn("not been built", guide.lower())
        self.assertIn("E remains", guide)

    def test_modal_bounds_fit_1280x720_and_1920x1080(self):
        hud = (ROOT / "src/game/client/hl2/eternalcity_hud.cpp").read_text()
        self.assertIn("MIN( kPanelWidth, width - 32 )", hud)
        self.assertIn("MIN( kPanelHeight, height - 48 )", hud)
        width = int(re.search(r"kPanelWidth = (\d+)", hud).group(1))
        height = int(re.search(r"kPanelHeight = (\d+)", hud).group(1))
        for screen_width, screen_height in ((1280, 720), (1920, 1080)):
            panel_width = min(width, screen_width - 32)
            panel_height = min(height, screen_height - 48)
            self.assertGreaterEqual((screen_width - panel_width) // 2, 16)
            self.assertGreaterEqual((screen_height - panel_height) // 2, 24)


if __name__ == "__main__":
    unittest.main()
