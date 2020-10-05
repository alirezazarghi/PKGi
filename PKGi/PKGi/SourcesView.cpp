#include "SourcesView.h"
#include "App.h"
#include "Graphics.h"
#include "Controller.h"
#include "Resource.h"
#include "Network.h"
#include "TinyJson.h"

#include "PackageListView.h"
#include "CreditView.h"

// Load resource for the view
SourcesView::SourcesView(Application* app)
{
	int rc;
	App = app;
	if (!App) { printf("[ERROR] App is null !\n"); return; }

	// Setup variable
	sources = new Source[2];
	snprintf(sources[0].name, 100, "%s", "Source of TheoryWrong");
	snprintf(sources[0].api_url, 100, "%s", "https://theorywrong.me/pkgs/api.php");
	snprintf(sources[1].name, 100, "%s", "Error-type source");
	snprintf(sources[1].api_url, 100, "%s", "https://test.fr/pkgs/pkgs.php");

	sourceNbr = 2;
	sourceSelected = 0;

	memset(errorMessage, 0, 255);
	onError = false;

	fetchStepAnim = 0;
	fetchTimeStep = 0;

	scePthreadMutexInit(&sources_mtx, NULL, "PKGiSourceMTX");

	// Set colors
	bgColor = { 0, 0, 0, 255 };
	fgColor = { 255, 255, 255, 255 };
}

// Unload resource
SourcesView::~SourcesView()
{
	// Cleanup memory
	scePthreadMutexLock(&sources_mtx);
	if (sources) {
		free(sources);
	}
	scePthreadMutexUnlock(&sources_mtx);

	// Destroy mutex
	scePthreadMutexDestroy(&sources_mtx);
}

// Show a message if error occurs
void SourcesView::ShowError(const char* error) {
	snprintf(errorMessage, sizeof(errorMessage), "%s", error);
	onError = true;
}

int SourcesView::Update() {
	// If mutex is locked, sources fetch is in progress
	if (scePthreadMutexTrylock(&sources_mtx) >= 0) {
		if (onError) {
			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CROSS)) {
				onError = false;
			}
		}
		else {
			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_UP)) {
				if ((sourceSelected - 1) >= 0) {
					sourceSelected--;
				}
			}

			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_DOWN)) {
				if ((sourceSelected + 1) < sourceNbr) {
					sourceSelected++;
				}
			}

			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_TRIANGLE)) {
				CreditView* cdt_view = new CreditView(App);
				App->ChangeView(cdt_view);
				scePthreadMutexUnlock(&sources_mtx);
				delete this;
				return 0;
			}

			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CROSS)) {
				if (sources && sourceSelected >= 0) {
					// Goto PackageListView
					PackageListArg args;
					snprintf(args.title, 100, "%s", sources[sourceSelected].name);
					snprintf(args.api_url, 100, "%s", sources[sourceSelected].api_url);
					PackageListView* pkgs_view = new PackageListView(App, &args);
					App->ChangeView(pkgs_view);
					scePthreadMutexUnlock(&sources_mtx);
					delete this;
					return 0;
				}
			}
		}

		// Unlock the mutex
		scePthreadMutexUnlock(&sources_mtx);
	}

	return 0;
}

int SourcesView::Render() {
	int ScreenHeight = App->Graph->GetScreenHeight();
	int ScreenWidth = App->Graph->GetScreenWidth();

	// Setup color
	Color notActive_color = { 0xA6, 0xA6, 0xA6, 0xFF };
	Color active_color = { 0x05, 0x2F, 0x7E, 0xFF };
	Color buttonText_color = { 0x0, 0x0, 0x0, 0xFF };
	Color green = { 0x0, 0xFF, 0x00, 0xFF };
	Color white = { 0xFF, 0xFF, 0xFF, 0xFF };
	Color dark = { 0x0, 0x00, 0x00, 0xFF };
	Color grey = { 0x40, 0x40, 0x40, 0xFF };
	Color orange = { 0xE3, 0x8A, 0x1E, 0xFF };

	// Calculate drawable package
	int menu_x = BORDER_X;
	int menu_y = HEADER_SIZE + MENU_BORDER;
	int menu_y_end = ScreenHeight - FOOTER_SIZE;

	int menu_size_height = menu_y_end - menu_y;
	int menu_size_width = ScreenWidth - (BORDER_X * 2);

	int consumable_border = (MENU_BORDER * (MENU_NBR_PER_PAGE - 2));
	int source_line_height = (menu_size_height - consumable_border) / MENU_NBR_PER_PAGE;

	int delta = sourceSelected / MENU_NBR_PER_PAGE;
	int maxPage = sourceNbr / MENU_NBR_PER_PAGE;

	char currentPageStr[255];
	sprintf(currentPageStr, "Page %i / %i", delta, maxPage);

	// Draw logo
	int logo_y = (HEADER_SIZE / 2) - (120 / 2);
	App->Graph->drawPNG(&App->Res->logo, BORDER_X, logo_y);

	// Draw source name
	char titleName[20] = "Sources List";
	FontSize titleSize;
	App->Graph->setFontSize(App->Res->robotoFont, 54);
	App->Graph->getTextSize(titleName, App->Res->robotoFont, &titleSize);
	//App->Graph->drawText(titleName, App->Res->robotoFont, BORDER_X + 120 + BORDER_X, HEADER_SIZE - 40 - titleSize.height, dark, white);
	App->Graph->drawText(titleName, App->Res->robotoFont, BORDER_X + 120 + BORDER_X, (logo_y + ((120 / 2) - (titleSize.height / 2))), dark, white);
	App->Graph->setFontSize(App->Res->robotoFont, DEFAULT_FONT_SIZE);

	// Draw header border
	App->Graph->drawRectangle(BORDER_X, HEADER_SIZE - 5, ScreenWidth - (BORDER_X * 2), 5, white);

	// If mutex is locked, fetch is in progress
	if (scePthreadMutexTrylock(&sources_mtx) < 0) {
		char progress[255] = "Fetching sources list .";

		uint64_t anim_time = sceKernelGetProcessTime();

		// stepAnim go from 0 to x each seconds
		if ((fetchTimeStep + 1000000) <= anim_time) {
			fetchTimeStep = anim_time;
			fetchStepAnim++;
			if (fetchStepAnim > 3)
				fetchStepAnim = 0;
		}

		if (fetchStepAnim == 0)
			snprintf(progress, 255, "%s.", progress);
		else if (fetchStepAnim == 1)
			snprintf(progress, 255, "%s..", progress);
		else if (fetchStepAnim == 1)
			snprintf(progress, 255, "%s...", progress);

		// Show the progress bar
		FontSize progressSize;
		App->Graph->getTextSize(progress, App->Res->robotoFont, &progressSize);

		int text_x = ((ScreenWidth / 2) - (progressSize.width / 2));
		int text_y = ((ScreenHeight / 2) - (progressSize.height / 2));
		App->Graph->drawText(progress, App->Res->robotoFont, text_x, text_y, dark, white);
	}
	else {
		fetchStepAnim = 0;

		// Mutex now locked, draw packages list
		if (onError) {
			FontSize errorSize;
			App->Graph->getTextSize(errorMessage, App->Res->robotoFont, &errorSize);
			App->Graph->drawText(errorMessage, App->Res->robotoFont, ((ScreenWidth / 2) - (errorSize.width / 2)), ((ScreenHeight / 2) - (errorSize.height / 2)), dark, white);
		}
		else if (sources) {
			// Draw packages
			for (int posMenu = 0; posMenu < MENU_NBR_PER_PAGE; posMenu++) {
				int currentSource = (delta * MENU_NBR_PER_PAGE) + posMenu;
				if (currentSource >= sourceNbr) {
					break;
				}

				Color selection = grey;
				if (sourceSelected == currentSource) {
					selection = orange;
				}

				FontSize titleSize;
				App->Graph->getTextSize(sources[currentSource].name, App->Res->robotoFont, &titleSize);

				int current_menu_y = menu_y + (posMenu * source_line_height) + (posMenu * MENU_BORDER);
				App->Graph->drawRectangle(menu_x, current_menu_y, menu_size_width, source_line_height, selection);

				int icon_size = source_line_height - (2 * MENU_IN_IMAGE_BORDER);
				App->Graph->drawSizedPNG(&App->Res->unknown, menu_x + MENU_IN_IMAGE_BORDER, current_menu_y + MENU_IN_IMAGE_BORDER, icon_size, icon_size);
				App->Graph->drawText(sources[currentSource].name, App->Res->robotoFont, menu_x + icon_size + (3 * MENU_IN_IMAGE_BORDER), current_menu_y + ((source_line_height / 2) - (titleSize.height / 2)), selection, white);
			}
		}
		else {
			FontSize ohnoSize;
			char ohno[255] = "No sources available :(";
			App->Graph->getTextSize(ohno, App->Res->robotoFont, &ohnoSize);
			App->Graph->drawText(ohno, App->Res->robotoFont, ((ScreenWidth / 2) - (ohnoSize.width / 2)), ((ScreenHeight / 2) - (ohnoSize.height / 2)), dark, white);
		}

		// Unlock the mutex
		scePthreadMutexUnlock(&sources_mtx);
	}

	// Draw footer background and border
	App->Graph->drawRectangle(0, ScreenHeight - FOOTER_SIZE, ScreenWidth, FOOTER_SIZE, dark);
	App->Graph->drawRectangle(BORDER_X, ScreenHeight - FOOTER_SIZE + 5, ScreenWidth - (BORDER_X * 2), 5, white);

	App->Graph->setFontSize(App->Res->robotoFont, FOOTER_TEXT_SIZE);

	char select[255] = "Select";
	char credit[255] = "Credit";

	FontSize selectSize;
	FontSize creditSize;

	App->Graph->getTextSize(select, App->Res->robotoFont, &selectSize);
	App->Graph->getTextSize(credit, App->Res->robotoFont, &creditSize);

	int icon_width = (selectSize.width + creditSize.width) + (2 * FOOTER_ICON_SIZE) + (2 * FOOTER_TEXT_BORDER);

	int icon_x = (ScreenWidth - BORDER_X) - icon_width;
	int icon_y = (ScreenHeight - FOOTER_SIZE + 5) + FOOTER_BORDER_Y;
	int text_y = icon_y + ((FOOTER_ICON_SIZE / 2) - (selectSize.height / 2));

	App->Graph->drawSizedPNG(&App->Res->cross, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
	icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
	App->Graph->drawText(select, App->Res->robotoFont, icon_x, text_y, dark, white);
	icon_x += selectSize.width + FOOTER_TEXT_SIZE;
	App->Graph->drawSizedPNG(&App->Res->triangle, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
	icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
	App->Graph->drawText(credit, App->Res->robotoFont, icon_x, text_y, dark, white);

	// Draw page number
	FontSize pageSize;
	App->Graph->getTextSize(currentPageStr, App->Res->robotoFont, &pageSize);
	App->Graph->drawText(currentPageStr, App->Res->robotoFont, BORDER_X, text_y, dark, white);

	App->Graph->setFontSize(App->Res->robotoFont, DEFAULT_FONT_SIZE);

	return 0;
}