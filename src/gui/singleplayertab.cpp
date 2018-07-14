/* This file is part of the Springlobby (GPL v2 or later), see COPYING */


#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/colordlg.h>
#include <wx/listctrl.h>
#include <wx/settings.h>
#include <wx/log.h>

#include <lslutils/conversion.h>

#include "singleplayertab.h"
#include "mapctrl.h"
#include "mapselectdialog.h"
#include "gui/controls.h"
#include "utils/conversion.h"
#include "uiutils.h"
#include "ui.h"
#include "hosting/addbotdialog.h"
#include "iserver.h"
#include "settings.h"
#include "gui/colorbutton.h"
#include "aui/auimanager.h"
#include "gui/customdialogs.h"
#include "utils/slpaths.h"
#include "utils/globalevents.h"
#include "log.h"
#include "utils/lslconversion.h"
#include "utils/slconfig.h"

BEGIN_EVENT_TABLE(SinglePlayerTab, wxPanel)

EVT_CHOICE(SP_MAP_PICK, SinglePlayerTab::OnMapSelect)
EVT_CHOICE(SP_MOD_PICK, SinglePlayerTab::OnModSelect)
EVT_CHOICE(SP_ENGINE_PICK, SinglePlayerTab::OnEngineSelect)
EVT_BUTTON(SP_BROWSE_MAP, SinglePlayerTab::OnMapBrowse)
EVT_BUTTON(SP_ADD_BOT, SinglePlayerTab::OnAddBot)
EVT_BUTTON(SP_RESET, SinglePlayerTab::OnReset)
EVT_BUTTON(SP_START, SinglePlayerTab::OnStart)
EVT_CHECKBOX(SP_RANDOM, SinglePlayerTab::OnRandomCheck)
EVT_CHECKBOX(SP_SPECTATE, SinglePlayerTab::OnSpectatorCheck)
EVT_BUTTON(SP_COLOUR, SinglePlayerTab::OnColorButton)

END_EVENT_TABLE()


int GetLastItemIndex(wxChoice* choice) {
	return static_cast<int>(choice->GetCount()) - 1;
}


SinglePlayerTab::SinglePlayerTab(wxWindow* parent, MainSinglePlayerTab& msptab)
    : wxPanel(parent, -1)
    , m_battle(msptab)
{
	GetAui().manager->AddPane(this, wxLEFT, _T("singleplayertab"));

	m_main_sizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* m_mapabour_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer* m_map_sizer = new wxBoxSizer(wxHORIZONTAL);
	// m_map_sizer->SetMinSize( wxSize( 352, -1 ) );


	// empty panel to replace minimap
	m_nominimap = new wxWindow(this, -1, wxDefaultPosition, wxSize(100, 100), wxSIMPLE_BORDER | wxFULL_REPAINT_ON_RESIZE);

	m_nominimap->SetToolTip(_("No Unitsync configured."));
	m_nominimap->SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	m_nominimap->SetBackgroundColour(*wxLIGHT_GREY);
	m_nominimap->Hide();
	m_map_sizer->Add(m_nominimap, 1, wxALL | wxEXPAND, 2);

//	Regular minimap

	m_minimap = new MapCtrl(this, 100, &m_battle, false, true, true);
	m_minimap->SetToolTip(_("You can drag the sun/bot icon around to define start position.\n "
				"Hover over the icon for a popup that lets you change side, ally and bonus."));
	m_map_sizer->Add(m_minimap, 1, wxALL | wxEXPAND, 2);
	m_mapabour_sizer->Add(m_map_sizer, 1, wxEXPAND, 2);

	//map description and parametrs like in battletab
	wxBoxSizer* m_opts_sizer = new wxBoxSizer(wxVERTICAL);

	m_map_opts_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(150, 160), wxLC_NO_HEADER | wxLC_REPORT);
	m_map_opts_list->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	m_map_opts_list->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_LIGHT));

	wxListItem col;

	col.SetText(_("Option"));
	m_map_opts_list->InsertColumn(0, col);
	col.SetText(_("Value"));
	m_map_opts_list->InsertColumn(1, col);
	m_map_opts_list->SetColumnWidth(0, 90);
	m_map_opts_list->SetColumnWidth(1, 50);

	m_map_opts_list->InsertItem(0, _("Size"));
	m_map_opts_list->InsertItem(1, _("Windspeed"));
	m_map_opts_list->InsertItem(2, _("Tidal strength"));
	m_map_opts_list->InsertItem(3, _("Gravity"));
	m_map_opts_list->InsertItem(4, _("Extractor radius"));
	m_map_opts_list->InsertItem(5, _("Max metal"));

	m_opts_sizer->Add(m_map_opts_list, 0, wxALL, 2);

	m_map_desc = new wxStaticText(this, -1, wxEmptyString);
	m_map_desc->Wrap(160);

	m_opts_sizer->Add(m_map_desc, 0, wxALL, 2);
	m_mapabour_sizer->Add(m_opts_sizer, 0, wxALL | wxEXPAND, 2);
	m_main_sizer->Add(m_mapabour_sizer, 1, wxEXPAND, 5);

	wxBoxSizer* m_ctrl_sizer = new wxBoxSizer(wxHORIZONTAL);

	m_mod_lbl = new wxStaticText(this, -1, _("Game:"));
	m_ctrl_sizer->Add(m_mod_lbl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	m_game_choice = new wxChoice(this, SP_MOD_PICK);
	m_game_choice->SetToolTip(_("No games? Download them by joining a multiplayer game room"));
	m_ctrl_sizer->Add(m_game_choice, 1, wxALL, 5);

	m_map_lbl = new wxStaticText(this, -1, _("Map:"));
	m_ctrl_sizer->Add(m_map_lbl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	m_map_pick = new wxChoice(this, SP_MAP_PICK);
	m_map_pick->SetToolTip(_("No maps? Download them by joining a multiplayer game room"));
	m_ctrl_sizer->Add(m_map_pick, 1, wxALL, 5);

	m_select_btn = new wxButton(this, SP_BROWSE_MAP, _("Choose map..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_ctrl_sizer->Add(m_select_btn, 0, wxBOTTOM | wxRIGHT | wxTOP, 5);

	m_mod_lbl = new wxStaticText(this, -1, _("Engine:"));
	m_ctrl_sizer->Add(m_mod_lbl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	m_engine_choice = new wxChoice(this, SP_ENGINE_PICK);
	m_engine_choice->SetToolTip(_("No engines? Download them by joining a multiplayer game room"));
	m_ctrl_sizer->Add(m_engine_choice, 1, wxALL, 5);

	//  m_ctrl_sizer->Add( 0, 0, 1, wxEXPAND, 0 );

	m_addbot_btn = new wxButton(this, SP_ADD_BOT, _("Add bot..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_ctrl_sizer->Add(m_addbot_btn, 0, wxALL, 5);

	m_main_sizer->Add(m_ctrl_sizer, 0, wxEXPAND, 5);

	m_buttons_sep = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
	m_main_sizer->Add(m_buttons_sep, 0, wxLEFT | wxRIGHT | wxEXPAND, 5);

	wxBoxSizer* m_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);

	m_buttons_sizer->Add(0, 0, 1, wxEXPAND, 0);

	m_color_btn = new ColorButton(this, SP_COLOUR, sett().GetBattleLastColour(), wxDefaultPosition, wxSize(30, CONTROL_HEIGHT));
	m_buttons_sizer->Add(m_color_btn, 0, wxALIGN_CENTER_VERTICAL | wxALL, 0);

	m_spectator_check = new wxCheckBox(this, SP_SPECTATE, _("Spectate only"));
	m_buttons_sizer->Add(m_spectator_check, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	m_random_check = new wxCheckBox(this, SP_RANDOM, _("Random start positions"));
	m_buttons_sizer->Add(m_random_check, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	m_start_btn = new wxButton(this, SP_START, _("Start"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_buttons_sizer->Add(m_start_btn, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	m_main_sizer->Add(m_buttons_sizer, 0, wxEXPAND, 5);

	m_battle.SetEngineName("spring");
	m_battle.SetEngineVersion(SlPaths::GetCurrentUsedSpringIndex());

	ReloadMaplist();
//	ReloadModlist(); //Called from ReloadEngineList() too
	ReloadEngineList();
	SUBSCRIBE_GLOBAL_EVENT(GlobalEventManager::OnUnitsyncReloaded, SinglePlayerTab::OnUnitsyncReloaded);

	this->SetSizer(m_main_sizer);
	this->Layout();
}


SinglePlayerTab::~SinglePlayerTab()
{
	GlobalEventManager::Instance()->UnSubscribeAll(this);
	wxDELETE(m_minimap);
}


void SinglePlayerTab::UpdateMinimap()
{
	m_minimap->UpdateMinimap();
}


void SinglePlayerTab::ReloadMaplist()
{
	m_map_pick->Clear();
	m_map_pick->Append(lslTowxArrayString(LSL::usync().GetMapList()));
	m_map_pick->Insert(_("-- Select one --"), m_map_pick->GetCount());

	const wxString mapName(m_battle.GetHostMapName());
	if (mapName.empty()) {
		SetMap(GetLastItemIndex(m_map_pick));
	} else {
		SetMap(m_map_pick->FindString(mapName, false /*SetStringSelection was case-insensitive*/));
	}
}


void SinglePlayerTab::ReloadModlist()
{
	m_game_choice->Clear();
	m_game_choice->Append(lslTowxArrayString(LSL::usync().GetGameList()));
	m_game_choice->Insert(_("-- Select one --"), m_game_choice->GetCount());

	const wxString gameName(m_battle.GetHostGameName());
	if (gameName.empty()) {
		SetGame(GetLastItemIndex(m_game_choice));
	} else {
		SetGame(m_game_choice->FindString(gameName, false /*SetStringSelection was case-insensitive*/));
	}
}


void SinglePlayerTab::ReloadEngineList()
{
	SlPaths::ValidatePaths();

	m_engine_choice->Clear();

	std::map<std::string, LSL::SpringBundle> versions = SlPaths::GetSpringVersionList();
	const std::string lastUsedEngineIndex = SlPaths::GetCurrentUsedSpringIndex();
	int i = 0;

	for (auto pair : versions) {
		m_engine_choice->Insert(TowxString(pair.first), i);
		if (lastUsedEngineIndex == pair.first) {
			m_engine_choice->SetSelection(i);
		}
		i++;
	}

	if (m_engine_choice->GetSelection() == wxNOT_FOUND) {
		m_engine_choice->SetSelection(0);
	}

	// i > 0 means that we have at least one engine.
	if (i == 0) {
		m_minimap->Hide();
		m_nominimap->Show();
	} else {
		m_nominimap->Hide();
		m_minimap->Show();
	}
	//unitsync change needs a refresh of games as well
	ReloadModlist(); // calls CheckForValidGameMapEngineTuple() at end anyway.
}


void SinglePlayerTab::SetEngine(int index)
{
	if (index == wxNOT_FOUND) {
		wxLogError("Invalid index selected: %d > %d", index, m_engine_choice->GetCount());
		return;
	}

	const std::string selection(STD_STRING(m_engine_choice->GetString(index)));
	wxLogMessage("Selected engine version %s", selection.c_str());

	SlPaths::SetUsedSpringIndex(selection);
	m_battle.SetEngineVersion(selection);
	LSL::usync().ReloadUnitSyncLib();

	CheckForValidGameMapEngineTuple();
}


void SinglePlayerTab::SetMap(int index)
{
	//ui().ReloadUnitSync();
	if (index == wxNOT_FOUND || index >= GetLastItemIndex(m_map_pick)) {
		index = GetLastItemIndex(m_map_pick);

		m_battle.SetHostMap("", "");
		int count = m_map_opts_list->GetItemCount();
		for (int i = 0; i < count; i++)
			m_map_opts_list->SetItem(i, 1, wxEmptyString);
		m_map_desc->SetLabel(wxEmptyString);
	} else {
		try {
			LSL::UnitsyncMap map = LSL::usync().GetMap(index);
			m_battle.SetHostMap(map.name, map.hash);
			m_map_opts_list->SetItem(0, 1, wxString::Format(_T( "%dx%d" ), map.info.width / 512, map.info.height / 512));
			m_map_opts_list->SetItem(1, 1, wxString::Format(_T( "%d-%d" ), map.info.minWind, map.info.maxWind));
			m_map_opts_list->SetItem(2, 1, wxString::Format(_T( "%d" ), map.info.tidalStrength));
			m_map_opts_list->SetItem(3, 1, wxString::Format(_T( "%d" ), map.info.gravity));
			m_map_opts_list->SetItem(4, 1, wxString::Format(_T( "%d" ), map.info.extractorRadius));
			m_map_opts_list->SetItem(5, 1, wxString::Format(_T( "%.3f" ), map.info.maxMetal));
			m_map_desc->SetLabel(TowxString(map.info.description));
			m_map_desc->Wrap(160);
			m_battle.SendHostInfo(IBattle::HI_Map_Changed); // reload map options
		} catch (...) {
		}
	}
	m_minimap->UpdateMinimap();
	m_map_pick->SetSelection(index);

	CheckForValidGameMapEngineTuple();
}


void SinglePlayerTab::ResetUsername()
{
	m_battle.GetMe().SetNick(STD_STRING(cfg().ReadString("/Spring/DefaultName")));
}


void SinglePlayerTab::SetGame(int index)
{
	//ui().ReloadUnitSync();

	size_t oldNumBots = m_battle.GetNumBots();
	if (index == wxNOT_FOUND || index >= GetLastItemIndex(m_game_choice)) {
		index = GetLastItemIndex(m_game_choice);

		m_battle.SetHostGame("", "");
	} else {
		try {
			LSL::UnitsyncGame mod = LSL::usync().GetGame(index);
			m_battle.SetLocalGame(mod);
			m_battle.SetHostGame(mod.name, mod.hash);
			m_battle.SendHostInfo(IBattle::HI_Restrictions); // Update restrictions in options.
			m_battle.SendHostInfo(IBattle::HI_Game_Changed); // reload mod options
		} catch (...) {
		}
	}
	m_minimap->UpdateMinimap();
	m_game_choice->SetSelection(index);

	if (oldNumBots != m_battle.GetNumBots())
		customMessageBoxModal(SL_MAIN_ICON, _("Incompatible bots have been removed after game selection changed."), _("Bots removed"));

	CheckForValidGameMapEngineTuple();
}


// GetSelection() returns selection after user closes the selection dropdown
// GetCurrentSelection() returns the one being selected if the dropdown is open, otherwise the above
bool HasValidSelection(wxChoice* picker)
{
	const int index = picker->GetCurrentSelection();
	if (index == wxNOT_FOUND)
		return false;
	// -- Select X -- is added to the end of pickers
	if (index >= GetLastItemIndex(picker))
		return false;

	return true;
}


bool SinglePlayerTab::CheckForValidGameMapEngineTuple()
{
	const bool isValid = HasValidSelection(m_game_choice)
	                  && HasValidSelection(m_map_pick)
	                  && (wxNOT_FOUND != m_engine_choice->GetCurrentSelection());

	if (isValid) {
		m_addbot_btn->Enable(true);
		m_start_btn->Enable(true);
		m_addbot_btn->SetToolTip(_("First select game, map and engine"));
		m_start_btn->SetToolTip(_("First select game, map and engine"));
	} else {
		m_addbot_btn->Enable(false);
		m_start_btn->Enable(false);
		m_addbot_btn->SetToolTip(wxEmptyString);
		m_start_btn->SetToolTip(wxEmptyString);
	}
	return isValid;
}


// 2/3 of these checks are now redundant
bool SinglePlayerTab::ValidSetup() const
{
	if (HasValidSelection(m_game_choice)) {
		wxLogWarning(_T("no game selected"));
		customMessageBox(SL_MAIN_ICON, _("You have to select a game first."), _("Game setup error"));
		return false;
	}

	if (HasValidSelection(m_map_pick)) {
		wxLogWarning(_T("no map selected"));
		customMessageBox(SL_MAIN_ICON, _("You have to select a map first."), _("Game setup error"));
		return false;
	}

	if (m_battle.GetNumUsers() == 1) {
		wxLogWarning(_T("Starting singleplayer game with no opponents"));
	}
	return true;
}


void SinglePlayerTab::OnMapSelect(wxCommandEvent& /*unused*/)
{
	SetMap(m_map_pick->GetCurrentSelection());
}


void SinglePlayerTab::OnModSelect(wxCommandEvent& /*unused*/)
{
	SetGame(m_game_choice->GetCurrentSelection());
}


void SinglePlayerTab::OnEngineSelect(wxCommandEvent& /*event*/)
{
	SetEngine(m_engine_choice->GetSelection());
}


void SinglePlayerTab::OnMapBrowse(wxCommandEvent& /*unused*/)
{
	slLogDebugFunc("");
	const wxString mapname = mapSelectDialog();
	if (!mapname.empty()) {
		SetMap(m_map_pick->FindString(mapname, true /*case sensitive*/));
	}
}


void SinglePlayerTab::OnAddBot(wxCommandEvent& /*unused*/)
{
	AddBotDialog dlg(this, m_battle, true);
	if (dlg.ShowModal() == wxID_OK) {
		UserBattleStatus bs;
		bs.owner = m_battle.GetMe().GetNick();
		bs.aishortname = STD_STRING(dlg.GetAIShortName());
		bs.airawname = STD_STRING(dlg.GetAiRawName());
		bs.aiversion = STD_STRING(dlg.GetAIVersion());
		bs.aitype = dlg.GetAIType();
		bs.team = m_battle.GetFreeTeam();
		bs.ally = m_battle.GetFreeAlly();
		bs.colour = m_battle.GetNewColour();
		m_battle.OnBotAdded(STD_STRING(dlg.GetNick()), bs);
		m_minimap->UpdateMinimap();
	}
}


void SinglePlayerTab::OnUnitsyncReloaded(wxCommandEvent& /*data*/)
{
	try {
		ReloadMaplist();
		ReloadModlist();
		ReloadEngineList();
		UpdateMinimap();
	} catch (...) {
		slLogDebugFunc("");
		wxLogError(_T("unitsync reload sink failed"));
	}
	Layout();
}


void SinglePlayerTab::OnStart(wxCommandEvent& /*unused*/)
{
	slLogDebugFunc("SP: ");

	if (ui().IsSpringRunning()) {
		wxLogWarning(_T("You cannot start a game (engine) while another engine is already running"));
		customMessageBoxModal(SL_MAIN_ICON,
		  _("You cannot start a game (engine) while another engine is already running"),
		  _("Spring error"), wxICON_EXCLAMATION);
		return;
	}

	if (ValidSetup())
		m_battle.StartSpring();
}


void SinglePlayerTab::OnRandomCheck(wxCommandEvent& /*unused*/)
{
	if (m_random_check->IsChecked())
		m_battle.CustomBattleOptions().setSingleOption("startpostype",
							       LSL::Util::ToIntString(IBattle::ST_Random), LSL::Enum::EngineOption);
	else
		m_battle.CustomBattleOptions().setSingleOption("startpostype",
							       LSL::Util::ToIntString(IBattle::ST_Pick), LSL::Enum::EngineOption);
	m_battle.SendHostInfo(IBattle::HI_StartType);
}


void SinglePlayerTab::OnSpectatorCheck(wxCommandEvent& /*unused*/)
{
	m_battle.GetMe().BattleStatus().spectator = m_spectator_check->IsChecked();
	UpdateMinimap();
}


void SinglePlayerTab::OnColorButton(wxCommandEvent& /*unused*/)
{
	User& u = m_battle.GetMe();
	wxColour CurrentColour = lslTowxColour(u.BattleStatus().colour);
	CurrentColour = GetColourFromUser(this, CurrentColour);
	if (!CurrentColour.IsOk())
		return;
	sett().SetBattleLastColour(CurrentColour);
	m_battle.ForceColour(u, wxColourTolsl(CurrentColour));
	UpdateMinimap();
}


void SinglePlayerTab::UpdatePresetList()
{
}


void SinglePlayerTab::OnReset(wxCommandEvent& /*unused*/)
{
}
