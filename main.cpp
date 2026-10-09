#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>
#include <ctime>

#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
#pragma comment(lib, "comctl32.lib")

using namespace std;

// ==================== КЛАССЫ ДАННЫХ ====================
class MenuItem {
public:
    int id;
    string name;
    string category;
    double price;
    int prepTime;
    string kitchenNotes;

    MenuItem(int i = 0, string n = "", string cat = "", double p = 0.0, int time = 10, string notes = "")
        : id(i), name(n), category(cat), price(p), prepTime(time), kitchenNotes(notes) {}
};

class Table {
public:
    int number;
    int capacity;
    bool isOccupied;
    string location;

    Table(int num = 0, int cap = 4, string loc = "Зал")
        : number(num), capacity(cap), isOccupied(false), location(loc) {}
};

class OrderItem {
public:
    MenuItem item;
    string status;
    time_t startTime;
    time_t finishTime;
    string chefNotes;

    OrderItem(const MenuItem& menuItem)
        : item(menuItem), status("В ожидании"), startTime(0), finishTime(0), chefNotes("") {}
};

class Order {
public:
    int id;
    int tableNumber;
    vector<OrderItem> items;
    string status;
    time_t createdTime;
    time_t kitchenAcceptTime;
    string waiterNotes;
    int currentCookingIndex; // Новое поле: индекс текущего готовящегося блюда

    Order(int i = 0, int tableNum = 0)
        : id(i), tableNumber(tableNum), status("Новый"), createdTime(time(0)),
          kitchenAcceptTime(0), waiterNotes(""), currentCookingIndex(-1) {}

    double getTotal() const {
        double total = 0;
        for (const auto& orderItem : items) {
            total += orderItem.item.price;
        }
        return total;
    }

    void addItem(const MenuItem& item) {
        items.push_back(OrderItem(item));
    }

    bool isReady() const {
        for (const auto& orderItem : items) {
            if (orderItem.status != "Готово" && orderItem.status != "Подано") {
                return false;
            }
        }
        return true;
    }

    bool isServed() const {
        for (const auto& orderItem : items) {
            if (orderItem.status != "Подано") {
                return false;
            }
        }
        return true;
    }

    bool canBeCompleted() const {
        return isServed() && status != "Завершен";
    }

    // Новый метод: получение индекса следующего блюда для приготовления
    int getNextItemToCook() const {
        for (int i = 0; i < items.size(); i++) {
            if (items[i].status == "В ожидании") {
                return i;
            }
        }
        return -1; // Нет блюд для приготовления
    }

    // Новый метод: получение индекса следующего готового блюда для подачи
    int getNextItemToServe() const {
        for (int i = 0; i < items.size(); i++) {
            if (items[i].status == "Готово") {
                return i;
            }
        }
        return -1; // Нет готовых блюд
    }
};

// ==================== МОДУЛЬ КУХНИ И ФИНАНСОВ ====================
class KitchenManager {
private:
    vector<Order*> activeOrders;
    vector<string> kitchenLog;
    string currentChef;
    int ordersCompletedToday;
    double dailyRevenue;

public:
    KitchenManager() : ordersCompletedToday(0), dailyRevenue(0.0) {
        currentChef = "Шеф-повар Иванов";
        kitchenLog.push_back("Кухня запущена");
    }

    void acceptOrder(Order* order) {
        if (order && order->status == "Новый") {
            order->status = "Передан на кухню";
            order->kitchenAcceptTime = time(0);
            activeOrders.push_back(order);

            string logEntry = "Заказ #" + to_string(order->id) + " принят на кухню";
            kitchenLog.push_back(logEntry);
        }
    }

    void startCookingItem(Order* order, int itemIndex) {
        if (order && itemIndex >= 0 && itemIndex < order->items.size()) {
            order->items[itemIndex].status = "Готовится";
            order->items[itemIndex].startTime = time(0);
            order->status = "Готовится";
            order->currentCookingIndex = itemIndex;

            string logEntry = "Начато приготовление: " + order->items[itemIndex].item.name +
                            " (Заказ #" + to_string(order->id) + ")";
            kitchenLog.push_back(logEntry);
        }
    }

    // Новый метод: автоматический старт приготовления следующего блюда
    bool startNextItem(Order* order) {
        if (!order) return false;

        int nextIndex = order->getNextItemToCook();
        if (nextIndex >= 0) {
            startCookingItem(order, nextIndex);
            return true;
        }
        return false;
    }

    void completeItem(Order* order, int itemIndex, const string& chefNotes = "") {
        if (order && itemIndex >= 0 && itemIndex < order->items.size()) {
            order->items[itemIndex].status = "Готово";
            order->items[itemIndex].finishTime = time(0);
            order->items[itemIndex].chefNotes = chefNotes;
            order->currentCookingIndex = -1;

            string logEntry = "Готово: " + order->items[itemIndex].item.name +
                            " (Заказ #" + to_string(order->id) + ")";
            kitchenLog.push_back(logEntry);

            // Автоматически начинаем готовить следующее блюдо, если есть
            if (startNextItem(order)) {
                logEntry = "Автоматически начато следующее блюдо (Заказ #" + to_string(order->id) + ")";
                kitchenLog.push_back(logEntry);
            }

            if (order->isReady()) {
                order->status = "Готов";
                string completeLog = "Заказ #" + to_string(order->id) + " полностью готов";
                kitchenLog.push_back(completeLog);
            }
        }
    }

    void markItemServed(Order* order, int itemIndex) {
        if (order && itemIndex >= 0 && itemIndex < order->items.size()) {
            order->items[itemIndex].status = "Подано";

            string logEntry = "Подано: " + order->items[itemIndex].item.name +
                            " (Заказ #" + to_string(order->id) + ")";
            kitchenLog.push_back(logEntry);

            bool allServed = true;
            for (const auto& item : order->items) {
                if (item.status != "Подано") {
                    allServed = false;
                    break;
                }
            }

            if (allServed) {
                order->status = "Подано";
                string serveLog = "Заказ #" + to_string(order->id) + " полностью подан";
                kitchenLog.push_back(serveLog);

                // Добавляем в выручку при полной подаче заказа
                dailyRevenue += order->getTotal();

                auto it = find(activeOrders.begin(), activeOrders.end(), order);
                if (it != activeOrders.end()) {
                    activeOrders.erase(it);
                    ordersCompletedToday++;
                }
            }
        }
    }

    // Новый метод: автоматическая подача следующего готового блюда
    bool serveNextItem(Order* order) {
        if (!order) return false;

        int nextIndex = order->getNextItemToServe();
        if (nextIndex >= 0) {
            markItemServed(order, nextIndex);
            return true;
        }
        return false;
    }

    vector<Order*> getActiveKitchenOrders() const {
        return activeOrders;
    }

    vector<string> getKitchenLog(int maxEntries = 20) const {
        if (kitchenLog.size() <= maxEntries) {
            return kitchenLog;
        }
        return vector<string>(kitchenLog.end() - maxEntries, kitchenLog.end());
    }

    map<string, double> getKitchenStatistics() const {
        map<string, double> stats;
        stats["orders_completed_today"] = ordersCompletedToday;
        stats["active_orders"] = activeOrders.size();
        stats["daily_revenue"] = dailyRevenue;

        int cookingItems = 0;
        int waitingItems = 0;
        int readyItems = 0;
        int servedItems = 0;

        for (const auto& order : activeOrders) {
            for (const auto& item : order->items) {
                if (item.status == "Готовится") cookingItems++;
                if (item.status == "В ожидании") waitingItems++;
                if (item.status == "Готово") readyItems++;
                if (item.status == "Подано") servedItems++;
            }
        }

        stats["items_cooking"] = cookingItems;
        stats["items_waiting"] = waitingItems;
        stats["items_ready"] = readyItems;
        stats["items_served"] = servedItems;

        return stats;
    }

    double getDailyRevenue() const {
        return dailyRevenue;
    }
};

// ==================== ОСНОВНОЙ МЕНЕДЖЕР ДАННЫХ ====================
class DataManager {
private:
    vector<Table> tables;
    vector<MenuItem> menu;
    vector<Order> orders;
    int nextOrderId;
    KitchenManager kitchen;

public:
    DataManager() : nextOrderId(1) {
        initializeData();
    }

    void initializeData() {
        tables.push_back(Table(1, 4, "Основной зал"));
        tables.push_back(Table(2, 2, "Основной зал"));
        tables.push_back(Table(3, 6, "VIP зал"));
        tables.push_back(Table(4, 4, "Терраса"));

        menu.push_back(MenuItem(1, "Пицца Маргарита", "Основные блюда", 450, 15, "Печь 250°C"));
        menu.push_back(MenuItem(2, "Салат Цезарь", "Салаты", 320, 5, "Соус цезарь"));
        menu.push_back(MenuItem(3, "Стейк Рибай", "Основные блюда", 890, 12, "Medium rare"));
        menu.push_back(MenuItem(4, "Тирамису", "Десерты", 280, 2, "Охлажденный"));
        menu.push_back(MenuItem(5, "Кола", "Напитки", 120, 1, "Со льдом"));
        menu.push_back(MenuItem(6, "Кофе Латте", "Напитки", 180, 3, "Двойной эспрессо"));
    }

    vector<Table> getTables() const { return tables; }
    vector<MenuItem> getMenu() const { return menu; }
    vector<Order> getOrders() const { return orders; }
    KitchenManager& getKitchen() { return kitchen; }

    Table* getTableByNumber(int number) {
        for (auto& table : tables) {
            if (table.number == number) return &table;
        }
        return nullptr;
    }

    MenuItem* getItemById(int id) {
        for (auto& item : menu) {
            if (item.id == id) return &item;
        }
        return nullptr;
    }

    Order* getOrderById(int id) {
        for (auto& order : orders) {
            if (order.id == id) return &order;
        }
        return nullptr;
    }

    int createOrder(int tableNumber, const string& waiterNotes = "") {
        Order newOrder(nextOrderId++, tableNumber);
        newOrder.waiterNotes = waiterNotes;
        orders.push_back(newOrder);

        Table* table = getTableByNumber(tableNumber);
        if (table) table->isOccupied = true;

        return newOrder.id;
    }

    void addItemToOrder(int orderId, int itemId) {
        Order* order = getOrderById(orderId);
        MenuItem* item = getItemById(itemId);

        if (order && item) {
            order->addItem(*item);
        }
    }

    void sendOrderToKitchen(int orderId) {
        Order* order = getOrderById(orderId);
        if (order && order->status == "Новый") {
            kitchen.acceptOrder(order);
            // Автоматически начинаем готовить первое блюдо
            kitchen.startNextItem(order);
        }
    }

    bool completeOrder(int orderId) {
        Order* order = getOrderById(orderId);
        if (order && order->canBeCompleted()) {
            order->status = "Завершен";
            Table* table = getTableByNumber(order->tableNumber);
            if (table) table->isOccupied = false;
            return true;
        }
        return false;
    }

    vector<Order> getActiveOrders() const {
        vector<Order> active;
        for (const auto& order : orders) {
            if (order.status != "Завершен") {
                active.push_back(order);
            }
        }
        return active;
    }

    vector<Order> getKitchenOrders() const {
        vector<Order> kitchenOrders;
        for (const auto& order : orders) {
            if (order.status == "Передан на кухню" || order.status == "Готовится" || order.status == "Готов" || order.status == "Подано") {
                kitchenOrders.push_back(order);
            }
        }
        return kitchenOrders;
    }

    double getTotalRevenue() const {
        return kitchen.getDailyRevenue();
    }
};

// ==================== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ====================
DataManager dataManager;
HWND hMainWindow;

HWND hTableList, hMenuList, hOrderList, hActiveOrdersList;
HWND hCreateOrderBtn, hAddItemBtn, hCompleteOrderBtn, hSendToKitchenBtn;
HWND hStatusText, hSwitchToKitchenBtn, hRevenueText;

HWND hKitchenOrdersList, hKitchenLogList, hKitchenStatsText;
HWND hStartCookingBtn, hCompleteItemBtn, hMarkServedBtn;
HWND hSwitchToHallBtn;

bool isKitchenMode = false;

// ==================== ПРОТОТИПЫ ФУНКЦИЙ ====================
void CreateHallControls(HWND hwnd);
void CreateKitchenControls(HWND hwnd);
void ShowHallInterface();
void ShowKitchenInterface();
void RefreshTableList();
void RefreshMenuList();
void RefreshActiveOrdersList();
void RefreshKitchenOrdersList();
void RefreshKitchenLog();
void RefreshKitchenStats();
void RefreshOrderDetails(int orderId);
void RefreshRevenue();
void UpdateStatus(const string& message);
void UpdateKitchenStatus(const string& message);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ==================== ГЛАВНАЯ ФУНКЦИЯ ====================
int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "RestaurantClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    hMainWindow = CreateWindow(
        "RestaurantClass",
        "Ресторан: Управление заказами",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 800,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hMainWindow, SW_SHOW);
    UpdateWindow(hMainWindow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// ==================== СОЗДАНИЕ ЭЛЕМЕНТОВ ИНТЕРФЕЙСА ====================
void CreateHallControls(HWND hwnd) {
    hSwitchToKitchenBtn = CreateWindow("BUTTON", "Перейти на кухню",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        700, 10, 200, 35, hwnd, (HMENU)10, NULL, NULL);

    CreateWindow("STATIC", "Столы:", WS_VISIBLE | WS_CHILD,
        12, 50, 200, 20, hwnd, NULL, NULL, NULL);
    hTableList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY,
        10, 70, 280, 150, hwnd, NULL, NULL, NULL);

    CreateWindow("STATIC", "Меню:", WS_VISIBLE | WS_CHILD,
        300, 50, 200, 20, hwnd, NULL, NULL, NULL);
    hMenuList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY,
        300, 70, 400, 150, hwnd, NULL, NULL, NULL);

    hCreateOrderBtn = CreateWindow("BUTTON", "Создать заказ",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        10, 230, 120, 35, hwnd, (HMENU)1, NULL, NULL);
    hAddItemBtn = CreateWindow("BUTTON", "Добавить блюдо",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        140, 230, 120, 35, hwnd, (HMENU)2, NULL, NULL);
    hSendToKitchenBtn = CreateWindow("BUTTON", "Передать на кухню",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        270, 230, 140, 35, hwnd, (HMENU)4, NULL, NULL);
    hCompleteOrderBtn = CreateWindow("BUTTON", "Завершить заказ",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        420, 230, 120, 35, hwnd, (HMENU)3, NULL, NULL);

    CreateWindow("STATIC", "Активные заказы:", WS_VISIBLE | WS_CHILD,
        10, 280, 200, 20, hwnd, NULL, NULL, NULL);
    hActiveOrdersList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY,
        10, 300, 690, 150, hwnd, NULL, NULL, NULL);

    CreateWindow("STATIC", "Детали заказа:", WS_VISIBLE | WS_CHILD,
        10, 460, 200, 20, hwnd, NULL, NULL, NULL);
    hOrderList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER,
        10, 480, 690, 150, hwnd, NULL, NULL, NULL);

    // Добавляем отображение выручки
    CreateWindow("STATIC", "Выручка за день:", WS_VISIBLE | WS_CHILD,
        700, 50, 200, 20, hwnd, NULL, NULL, NULL);
    hRevenueText = CreateWindow("STATIC", "0 руб.", WS_VISIBLE | WS_CHILD,
        700, 70, 200, 30, hwnd, NULL, NULL, NULL);

    hStatusText = CreateWindow("STATIC", "Статус: Готов к работе",
        WS_VISIBLE | WS_CHILD,
        10, 640, 800, 20, hwnd, NULL, NULL, NULL);

    RefreshTableList();
    RefreshMenuList();
    RefreshActiveOrdersList();
    RefreshRevenue();
}

void CreateKitchenControls(HWND hwnd) {
    hSwitchToHallBtn = CreateWindow("BUTTON", "Вернуться в зал",
        WS_CHILD | BS_PUSHBUTTON,
        700, 10, 200, 35, hwnd, (HMENU)11, NULL, NULL);

    CreateWindow("STATIC", "Заказы на кухне:", WS_CHILD,
        10, 50, 200, 20, hwnd, NULL, NULL, NULL);
    hKitchenOrdersList = CreateWindow("LISTBOX", "", WS_CHILD | WS_BORDER | LBS_NOTIFY,
        10, 70, 500, 200, hwnd, NULL, NULL, NULL);

    CreateWindow("STATIC", "Лог кухни:", WS_CHILD,
        520, 50, 200, 20, hwnd, NULL, NULL, NULL);
    hKitchenLogList = CreateWindow("LISTBOX", "", WS_CHILD | WS_BORDER,
        520, 70, 450, 200, hwnd, NULL, NULL, NULL);

    CreateWindow("STATIC", "Статистика кухни:", WS_CHILD,
        10, 280, 200, 20, hwnd, NULL, NULL, NULL);
    hKitchenStatsText = CreateWindow("STATIC", "", WS_CHILD,
        10, 300, 400, 100, hwnd, NULL, NULL, NULL);

    // Изменяем названия кнопок для лучшего понимания
    hStartCookingBtn = CreateWindow("BUTTON", "Начать след. блюдо",
        WS_CHILD | BS_PUSHBUTTON,
        10, 410, 150, 35, hwnd, (HMENU)5, NULL, NULL);
    hCompleteItemBtn = CreateWindow("BUTTON", "Завершить текущее",
        WS_CHILD | BS_PUSHBUTTON,
        170, 410, 150, 35, hwnd, (HMENU)6, NULL, NULL);
    hMarkServedBtn = CreateWindow("BUTTON", "Подать готовое",
        WS_CHILD | BS_PUSHBUTTON,
        330, 410, 150, 35, hwnd, (HMENU)7, NULL, NULL);
}

void ShowHallInterface() {
    ShowWindow(hSwitchToKitchenBtn, SW_SHOW);
    ShowWindow(hTableList, SW_SHOW);
    ShowWindow(hMenuList, SW_SHOW);
    ShowWindow(hCreateOrderBtn, SW_SHOW);
    ShowWindow(hAddItemBtn, SW_SHOW);
    ShowWindow(hSendToKitchenBtn, SW_SHOW);
    ShowWindow(hCompleteOrderBtn, SW_SHOW);
    ShowWindow(hActiveOrdersList, SW_SHOW);
    ShowWindow(hOrderList, SW_SHOW);
    ShowWindow(hStatusText, SW_SHOW);
    ShowWindow(hRevenueText, SW_SHOW);

    ShowWindow(hSwitchToHallBtn, SW_HIDE);
    ShowWindow(hKitchenOrdersList, SW_HIDE);
    ShowWindow(hKitchenLogList, SW_HIDE);
    ShowWindow(hKitchenStatsText, SW_HIDE);
    ShowWindow(hStartCookingBtn, SW_HIDE);
    ShowWindow(hCompleteItemBtn, SW_HIDE);
    ShowWindow(hMarkServedBtn, SW_HIDE);

    SetWindowTextA(hMainWindow, "Ресторан: Управление заказами - Зал");
}

void ShowKitchenInterface() {
    ShowWindow(hSwitchToKitchenBtn, SW_HIDE);
    ShowWindow(hTableList, SW_HIDE);
    ShowWindow(hMenuList, SW_HIDE);
    ShowWindow(hCreateOrderBtn, SW_HIDE);
    ShowWindow(hAddItemBtn, SW_HIDE);
    ShowWindow(hSendToKitchenBtn, SW_HIDE);
    ShowWindow(hCompleteOrderBtn, SW_HIDE);
    ShowWindow(hActiveOrdersList, SW_HIDE);
    ShowWindow(hOrderList, SW_HIDE);
    ShowWindow(hStatusText, SW_HIDE);
    ShowWindow(hRevenueText, SW_HIDE);

    RefreshKitchenOrdersList();
    RefreshKitchenLog();
    RefreshKitchenStats();

    ShowWindow(hSwitchToHallBtn, SW_SHOW);
    ShowWindow(hKitchenOrdersList, SW_SHOW);
    ShowWindow(hKitchenLogList, SW_SHOW);
    ShowWindow(hKitchenStatsText, SW_SHOW);
    ShowWindow(hStartCookingBtn, SW_SHOW);
    ShowWindow(hCompleteItemBtn, SW_SHOW);
    ShowWindow(hMarkServedBtn, SW_SHOW);

    SetWindowTextA(hMainWindow, "Ресторан: Управление заказами - Кухня");
}

// ==================== ОБНОВЛЕНИЕ ИНТЕРФЕЙСА ====================
void RefreshTableList() {
    SendMessage(hTableList, LB_RESETCONTENT, 0, 0);
    auto tables = dataManager.getTables();
    for (const auto& table : tables) {
        string status = table.isOccupied ? "Занят" : "Свободен";
        string item = "Стол " + to_string(table.number) + " (" + to_string(table.capacity) + " пер.) - " + status;
        SendMessageA(hTableList, LB_ADDSTRING, 0, (LPARAM)item.c_str());
    }
}

void RefreshMenuList() {
    SendMessage(hMenuList, LB_RESETCONTENT, 0, 0);
    auto menu = dataManager.getMenu();
    for (const auto& item : menu) {
        string listItem = item.name + " - " + to_string((int)item.price) + " руб. (" + to_string(item.prepTime) + " мин)";
        SendMessageA(hMenuList, LB_ADDSTRING, 0, (LPARAM)listItem.c_str());
    }
}

void RefreshActiveOrdersList() {
    SendMessage(hActiveOrdersList, LB_RESETCONTENT, 0, 0);
    auto orders = dataManager.getActiveOrders();
    for (const auto& order : orders) {
        string canComplete = order.canBeCompleted() ? " [МОЖНО ЗАВЕРШИТЬ]" : "";
        string item = "Заказ #" + to_string(order.id) + " | Стол: " + to_string(order.tableNumber) +
                     " | Статус: " + order.status + " | Сумма: " + to_string((int)order.getTotal()) + " руб." + canComplete;
        SendMessageA(hActiveOrdersList, LB_ADDSTRING, 0, (LPARAM)item.c_str());
    }
}

void RefreshKitchenOrdersList() {
    SendMessage(hKitchenOrdersList, LB_RESETCONTENT, 0, 0);
    auto orders = dataManager.getKitchenOrders();
    for (const auto& order : orders) {
        int cooking = 0, waiting = 0, ready = 0, served = 0;
        for (const auto& item : order.items) {
            if (item.status == "Готовится") cooking++;
            if (item.status == "В ожидании") waiting++;
            if (item.status == "Готово") ready++;
            if (item.status == "Подано") served++;
        }

        string item = "Заказ #" + to_string(order.id) + " | Стол: " + to_string(order.tableNumber) +
                     " | " + order.status + " | " + to_string(order.items.size()) + " блюд " +
                     "(Ожидание: " + to_string(waiting) + ", Готовится: " + to_string(cooking) +
                     ", Готово: " + to_string(ready) + ", Подано: " + to_string(served) + ")";
        SendMessageA(hKitchenOrdersList, LB_ADDSTRING, 0, (LPARAM)item.c_str());
    }
}

void RefreshKitchenLog() {
    SendMessage(hKitchenLogList, LB_RESETCONTENT, 0, 0);
    auto log = dataManager.getKitchen().getKitchenLog();
    for (const auto& entry : log) {
        SendMessageA(hKitchenLogList, LB_ADDSTRING, 0, (LPARAM)entry.c_str());
    }
    int count = SendMessage(hKitchenLogList, LB_GETCOUNT, 0, 0);
    if (count > 0) {
        SendMessage(hKitchenLogList, LB_SETCURSEL, count - 1, 0);
    }
}

void RefreshKitchenStats() {
    auto stats = dataManager.getKitchen().getKitchenStatistics();
    string statsText = "Заказов завершено сегодня: " + to_string((int)stats["orders_completed_today"]) + "\n" +
                      "Активных заказов: " + to_string((int)stats["active_orders"]) + "\n" +
                      "Блюд готовится: " + to_string((int)stats["items_cooking"]) + "\n" +
                      "Блюд в ожидании: " + to_string((int)stats["items_waiting"]) + "\n" +
                      "Блюд готово: " + to_string((int)stats["items_ready"]) + "\n" +
                      "Блюд подано: " + to_string((int)stats["items_served"]) + "\n" +
                      "Выручка за день: " + to_string((int)stats["daily_revenue"]) + " руб.";
    SetWindowTextA(hKitchenStatsText, statsText.c_str());
}

void RefreshRevenue() {
    double revenue = dataManager.getTotalRevenue();
    string revenueText = to_string((int)revenue) + " руб.";
    SetWindowTextA(hRevenueText, revenueText.c_str());
}

void RefreshOrderDetails(int orderId) {
    SendMessage(hOrderList, LB_RESETCONTENT, 0, 0);
    auto order = dataManager.getOrderById(orderId);
    if (order) {
        SendMessageA(hOrderList, LB_ADDSTRING, 0, (LPARAM)"=== ДЕТАЛИ ЗАКАЗА ===");
        SendMessageA(hOrderList, LB_ADDSTRING, 0, (LPARAM)("Статус: " + order->status).c_str());

        // Показываем текущее готовящееся блюдо
        if (order->currentCookingIndex >= 0 && order->currentCookingIndex < order->items.size()) {
            string cooking = "Сейчас готовится: " + order->items[order->currentCookingIndex].item.name;
            SendMessageA(hOrderList, LB_ADDSTRING, 0, (LPARAM)cooking.c_str());
        }

        for (int i = 0; i < order->items.size(); i++) {
            const auto& orderItem = order->items[i];
            string prefix = (i == order->currentCookingIndex) ? ">>> " : "";
            string listItem = prefix + orderItem.item.name + " - " +
                            to_string((int)orderItem.item.price) + " руб. [" + orderItem.status + "]";
            SendMessageA(hOrderList, LB_ADDSTRING, 0, (LPARAM)listItem.c_str());
        }
        string total = "ИТОГО: " + to_string((int)order->getTotal()) + " руб.";
        SendMessageA(hOrderList, LB_ADDSTRING, 0, (LPARAM)total.c_str());

        if (order->canBeCompleted()) {
            SendMessageA(hOrderList, LB_ADDSTRING, 0, (LPARAM)"=== ЗАКАЗ МОЖНО ЗАВЕРШИТЬ ===");
        } else {
            SendMessageA(hOrderList, LB_ADDSTRING, 0, (LPARAM)"=== НЕ ВСЕ БЛЮДА ПОДАНЫ ===");
        }
    }
}

void UpdateStatus(const string& message) {
    string status = "Статус: " + message;
    SetWindowTextA(hStatusText, status.c_str());
}

void UpdateKitchenStatus(const string& message) {
    string status = "Кухня: " + message;
    UpdateStatus(status);
}

// ==================== ОБРАБОТЧИК СОБЫТИЙ ====================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            CreateHallControls(hwnd);
            CreateKitchenControls(hwnd);
            ShowHallInterface();
            break;

        case WM_COMMAND: {
            int cmd = LOWORD(wParam);

            if (cmd == 10) {
                isKitchenMode = true;
                ShowKitchenInterface();
                break;
            }
            else if (cmd == 11) {
                isKitchenMode = false;
                ShowHallInterface();
                break;
            }

            if (!isKitchenMode) {
                if (cmd == 1) {
                    int tableIndex = SendMessage(hTableList, LB_GETCURSEL, 0, 0);
                    if (tableIndex >= 0) {
                        auto tables = dataManager.getTables();
                        if (tableIndex < tables.size() && !tables[tableIndex].isOccupied) {
                            int orderId = dataManager.createOrder(tables[tableIndex].number);
                            RefreshTableList();
                            RefreshActiveOrdersList();
                            UpdateStatus("Создан новый заказ для стола " + to_string(tables[tableIndex].number));
                        }
                    }
                }
                else if (cmd == 2) {
                    int orderIndex = SendMessage(hActiveOrdersList, LB_GETCURSEL, 0, 0);
                    int menuIndex = SendMessage(hMenuList, LB_GETCURSEL, 0, 0);

                    if (orderIndex >= 0 && menuIndex >= 0) {
                        auto orders = dataManager.getActiveOrders();
                        auto menu = dataManager.getMenu();

                        if (orderIndex < orders.size() && menuIndex < menu.size()) {
                            dataManager.addItemToOrder(orders[orderIndex].id, menu[menuIndex].id);
                            RefreshActiveOrdersList();
                            RefreshOrderDetails(orders[orderIndex].id);
                            UpdateStatus("Добавлено блюдо в заказ");
                        }
                    }
                }
                else if (cmd == 3) {
                    int orderIndex = SendMessage(hActiveOrdersList, LB_GETCURSEL, 0, 0);
                    if (orderIndex >= 0) {
                        auto orders = dataManager.getActiveOrders();
                        if (orderIndex < orders.size()) {
                            if (dataManager.completeOrder(orders[orderIndex].id)) {
                                RefreshTableList();
                                RefreshActiveOrdersList();
                                RefreshRevenue();
                                SendMessage(hOrderList, LB_RESETCONTENT, 0, 0);
                                UpdateStatus("Заказ завершен");
                            } else {
                                UpdateStatus("Ошибка: не все блюда поданы!");
                            }
                        }
                    }
                }
                else if (cmd == 4) {
                    int orderIndex = SendMessage(hActiveOrdersList, LB_GETCURSEL, 0, 0);
                    if (orderIndex >= 0) {
                        auto orders = dataManager.getActiveOrders();
                        if (orderIndex < orders.size()) {
                            dataManager.sendOrderToKitchen(orders[orderIndex].id);
                            RefreshActiveOrdersList();
                            RefreshKitchenOrdersList();
                            RefreshKitchenLog();
                            UpdateStatus("Заказ передан на кухню");
                        }
                    }
                }
                else if (HIWORD(wParam) == LBN_SELCHANGE) {
                    if ((HWND)lParam == hActiveOrdersList) {
                        int orderIndex = SendMessage(hActiveOrdersList, LB_GETCURSEL, 0, 0);
                        if (orderIndex >= 0) {
                            auto orders = dataManager.getActiveOrders();
                            if (orderIndex < orders.size()) {
                                RefreshOrderDetails(orders[orderIndex].id);
                            }
                        }
                    }
                }
            } else {
                if (cmd == 5) {
                    int orderIndex = SendMessage(hKitchenOrdersList, LB_GETCURSEL, 0, 0);
                    if (orderIndex >= 0) {
                        auto orders = dataManager.getKitchenOrders();
                        if (orderIndex < orders.size()) {
                            Order* order = dataManager.getOrderById(orders[orderIndex].id);
                            if (order) {
                                if (dataManager.getKitchen().startNextItem(order)) {
                                    RefreshKitchenOrdersList();
                                    RefreshKitchenLog();
                                    RefreshKitchenStats();
                                    UpdateKitchenStatus("Начато приготовление следующего блюда");
                                } else {
                                    UpdateKitchenStatus("Нет блюд для приготовления");
                                }
                            }
                        }
                    }
                }
                else if (cmd == 6) {
                    int orderIndex = SendMessage(hKitchenOrdersList, LB_GETCURSEL, 0, 0);
                    if (orderIndex >= 0) {
                        auto orders = dataManager.getKitchenOrders();
                        if (orderIndex < orders.size()) {
                            Order* order = dataManager.getOrderById(orders[orderIndex].id);
                            if (order) {
                                // Завершаем текущее готовящееся блюдо
                                if (order->currentCookingIndex >= 0 &&
                                    order->currentCookingIndex < order->items.size()) {
                                    dataManager.getKitchen().completeItem(order,
                                        order->currentCookingIndex,
                                        "Отлично приготовлено!");
                                    RefreshKitchenOrdersList();
                                    RefreshKitchenLog();
                                    RefreshKitchenStats();
                                    RefreshActiveOrdersList();
                                    UpdateKitchenStatus("Блюдо готово");
                                } else {
                                    // Ищем первое блюдо "Готовится"
                                    for (int i = 0; i < order->items.size(); i++) {
                                        if (order->items[i].status == "Готовится") {
                                            dataManager.getKitchen().completeItem(order, i,
                                                "Отлично приготовлено!");
                                            RefreshKitchenOrdersList();
                                            RefreshKitchenLog();
                                            RefreshKitchenStats();
                                            RefreshActiveOrdersList();
                                            UpdateKitchenStatus("Блюдо готово");
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else if (cmd == 7) {
                    int orderIndex = SendMessage(hKitchenOrdersList, LB_GETCURSEL, 0, 0);
                    if (orderIndex >= 0) {
                        auto orders = dataManager.getKitchenOrders();
                        if (orderIndex < orders.size()) {
                            Order* order = dataManager.getOrderById(orders[orderIndex].id);
                            if (order) {
                                if (dataManager.getKitchen().serveNextItem(order)) {
                                    RefreshKitchenOrdersList();
                                    RefreshKitchenLog();
                                    RefreshKitchenStats();
                                    RefreshActiveOrdersList();
                                    RefreshRevenue();
                                    UpdateKitchenStatus("Блюдо подано");
                                } else {
                                    UpdateKitchenStatus("Нет готовых блюд для подачи");
                                }
                            }
                        }
                    }
                }
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}