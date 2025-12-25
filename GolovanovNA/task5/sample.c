#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tchar.h>

#define MAX_PATH_LEN 1024
#define INITIAL_CAPACITY 16

typedef struct {
    TCHAR* name;
    LONGLONG size;
    BOOL is_directory;
    FILETIME creation_time;
    FILETIME modify_time;
} FileInfo;

FileInfo* files = NULL;
int file_count = 0;
int file_capacity = 0;

void simple_sort(FileInfo* arr, int n, BOOL ascending);
void selection_sort(FileInfo* arr, int n, BOOL ascending);
void insertion_sort(FileInfo* arr, int n, BOOL ascending);
void merge_sort(FileInfo* arr, int left, int right, BOOL ascending);
void merge(FileInfo* arr, int left, int mid, int right, BOOL ascending);
void quick_sort(FileInfo* arr, int left, int right, BOOL ascending);
int partition(FileInfo* arr, int left, int right, BOOL ascending);

void clear_screen();
int get_files_list(const TCHAR* path);
void print_files();
void print_welcome();
void print_sort_methods();
LONGLONG file_size_to_bytes(const WIN32_FIND_DATA* fd);
TCHAR* format_size(LONGLONG bytes);
void free_files_array();
int init_files_array();
int resize_files_array(int new_capacity);
void safe_swap_files(FileInfo* a, FileInfo* b);
void deep_copy_fileinfo(FileInfo* dest, const FileInfo* src);

double get_high_resolution_time() {
    LARGE_INTEGER frequency, time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&time);
    return (double)time.QuadPart / frequency.QuadPart;
}

void safe_swap_files(FileInfo* a, FileInfo* b) {
    if (a == b) return;

    FileInfo temp = *a;

    a->name = b->name;
    a->size = b->size;
    a->is_directory = b->is_directory;
    a->creation_time = b->creation_time;
    a->modify_time = b->modify_time;

    b->name = temp.name;
    b->size = temp.size;
    b->is_directory = temp.is_directory;
    b->creation_time = temp.creation_time;
    b->modify_time = temp.modify_time;
}

void deep_copy_fileinfo(FileInfo* dest, const FileInfo* src) {
    if (dest->name) {
        free(dest->name);
        dest->name = NULL;
    }

    if (src->name) {
        dest->name = (TCHAR*)malloc((_tcslen(src->name) + 1) * sizeof(TCHAR));
        if (dest->name) {
            _tcscpy_s(dest->name, _tcslen(src->name) + 1, src->name);
        }
    }

    dest->size = src->size;
    dest->is_directory = src->is_directory;
    dest->creation_time = src->creation_time;
    dest->modify_time = src->modify_time;
}

int _tmain() {
    TCHAR path[MAX_PATH_LEN] = _T("");
    int choice;
    int sort_method;
    BOOL ascending = TRUE;
    BOOL files_loaded = FALSE;
    double start_time, end_time, sort_time;

    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    if (!init_files_array()) {
        _tprintf(_T("Ошибка инициализации массива файлов!\n"));
        return 1;
    }

    while (1) {
        clear_screen();
        print_welcome();

        _tprintf(_T("\nТекущий каталог: %s\n"),
            _tcslen(path) > 0 ? path : _T("Не выбран"));
        _tprintf(_T("Количество файлов: %d\n"), file_count);
        _tprintf(_T("Емкость массива: %d\n"), file_capacity);
        _tprintf(_T("\n=== МЕНЮ ===\n"));
        _tprintf(_T("1. Выбрать каталог\n"));
        _tprintf(_T("2. Вывести список файлов (без сортировки)\n"));
        _tprintf(_T("3. Сортировать файлы\n"));
        _tprintf(_T("4. Сменить направление сортировки\n"));
        _tprintf(_T("5. Очистить память и выйти\n"));
        _tprintf(_T("Выберите действие: "));

        if (_tscanf(_T("%d"), &choice) != 1) {
            _tprintf(_T("Ошибка ввода!\n"));
            while (_gettchar() != _T('\n'));
            continue;
        }
        while (_gettchar() != _T('\n'));

        switch (choice) {
        case 1: {
            _tprintf(_T("\nВведите путь к каталогу: "));
            if (!_fgetts(path, MAX_PATH_LEN, stdin)) {
                _tprintf(_T("Ошибка чтения пути!\n"));
                break;
            }
            path[_tcsclen(path) - 1] = _T('\0');

            if (_tcslen(path) == 0) {
                _tprintf(_T("Путь не может быть пустым!\n"));
                break;
            }

            DWORD attributes = GetFileAttributes(path);
            if (attributes == INVALID_FILE_ATTRIBUTES ||
                !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                _tprintf(_T("Ошибка: каталог не существует или недоступен!\n"));
                path[0] = _T('\0');
                files_loaded = FALSE;
            }
            else {
                if (get_files_list(path)) {
                    files_loaded = TRUE;
                    _tprintf(_T("Загружено %d файлов и каталогов.\n"), file_count);
                }
                else {
                    _tprintf(_T("Ошибка загрузки файлов из каталога!\n"));
                    files_loaded = FALSE;
                }
            }
            break;
        }

        case 2:
            if (!files_loaded) {
                _tprintf(_T("Сначала выберите каталог!\n"));
            }
            else if (file_count == 0) {
                _tprintf(_T("Каталог пуст!\n"));
            }
            else {
                _tprintf(_T("\n=== Список файлов (без сортировки) ===\n"));
                print_files();
            }
            break;

        case 3:
            if (!files_loaded) {
                _tprintf(_T("Сначала выберите каталог!\n"));
                break;
            }

            if (file_count == 0) {
                _tprintf(_T("Каталог пуст!\n"));
                break;
            }

            print_sort_methods();
            _tprintf(_T("Выберите метод сортировки (1-5): "));
            if (_tscanf(_T("%d"), &sort_method) != 1) {
                _tprintf(_T("Неверный ввод!\n"));
                while (_gettchar() != _T('\n'));
                break;
            }
            while (_gettchar() != _T('\n'));

            if (sort_method < 1 || sort_method > 5) {
                _tprintf(_T("Неверный выбор метода сортировки!\n"));
                break;
            }

            start_time = get_high_resolution_time();

            switch (sort_method) {
            case 1:
                simple_sort(files, file_count, ascending);
                _tprintf(_T("Применена простейшая сортировка "));
                break;
            case 2:
                selection_sort(files, file_count, ascending);
                _tprintf(_T("Применена сортировка выбором "));
                break;
            case 3:
                insertion_sort(files, file_count, ascending);
                _tprintf(_T("Применена сортировка вставками "));
                break;
            case 4:
                merge_sort(files, 0, file_count - 1, ascending);
                _tprintf(_T("Применена сортировка слиянием "));
                break;
            case 5:
                quick_sort(files, 0, file_count - 1, ascending);
                _tprintf(_T("Применена быстрая сортировка "));
                break;
            }

            end_time = get_high_resolution_time();
            sort_time = (end_time - start_time) * 1000.0;

            _tprintf(_T("%s\n"), ascending ? _T("(по возрастанию)") : _T("(по убыванию)"));


            if (sort_time < 1.0) {
                _tprintf(_T("Время сортировки: %.3f микросекунд\n\n"), sort_time * 1000.0);
            }
            else if (sort_time < 1000.0) {
                _tprintf(_T("Время сортировки: %.3f миллисекунд\n\n"), sort_time);
            }
            else {
                _tprintf(_T("Время сортировки: %.3f секунд\n\n"), sort_time / 1000.0);
            }

            _tprintf(_T("=== Отсортированный список файлов ===\n"));
            print_files();
            break;

        case 4:
            ascending = !ascending;
            _tprintf(_T("Направление сортировки изменено на: %s\n"),
                ascending ? _T("по возрастанию") : _T("по убыванию"));
            break;

        case 5:
            _tprintf(_T("Очистка памяти и выход из программы.\n"));
            free_files_array();
            return 0;

        default:
            _tprintf(_T("Неверный выбор! Попробуйте снова.\n"));
        }

        _tprintf(_T("\nНажмите Enter для продолжения..."));
        while (_gettchar() != _T('\n'));
    }

    free_files_array();
    return 0;
}

int init_files_array() {
    file_capacity = INITIAL_CAPACITY;
    files = (FileInfo*)calloc(file_capacity, sizeof(FileInfo));
    if (!files) {
        _tprintf(_T("Ошибка выделения памяти!\n"));
        return 0;
    }
    file_count = 0;
    return 1;
}

int resize_files_array(int new_capacity) {
    if (new_capacity <= file_capacity) {
        return 1;
    }

    FileInfo* temp = (FileInfo*)calloc(new_capacity, sizeof(FileInfo));
    if (!temp) {
        _tprintf(_T("Ошибка перераспределения памяти!\n"));
        return 0;
    }

    for (int i = 0; i < file_count; i++) {
        temp[i] = files[i];
    }

    FileInfo* old_files = files;
    files = temp;
    free(old_files);

    file_capacity = new_capacity;
    return 1;
}

void free_files_array() {
    if (files) {
        for (int i = 0; i < file_count; i++) {
            if (files[i].name) {
                free(files[i].name);
                files[i].name = NULL;
            }
        }
        free(files);
        files = NULL;
        file_count = 0;
        file_capacity = 0;
    }
}

int get_files_list(const TCHAR* path) {
    TCHAR search_path[MAX_PATH_LEN + 3];
    WIN32_FIND_DATA find_data;
    HANDLE hFind;

    for (int i = 0; i < file_count; i++) {
        if (files[i].name) {
            free(files[i].name);
            files[i].name = NULL;
        }
    }
    file_count = 0;

    if (_tcslen(path) > MAX_PATH_LEN - 3) {
        _tprintf(_T("Путь слишком длинный!\n"));
        return 0;
    }

    _stprintf_s(search_path, MAX_PATH_LEN + 3, _T("%s\\*"), path);

    hFind = FindFirstFile(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND) {
            _tprintf(_T("Каталог пуст или недоступен.\n"));
            return 1;
        }
        _tprintf(_T("Ошибка FindFirstFile: %lu\n"), error);
        return 0;
    }

    do {
        if (_tcscmp(find_data.cFileName, _T(".")) == 0 ||
            _tcscmp(find_data.cFileName, _T("..")) == 0) {
            continue;
        }

        if (file_count >= file_capacity) {
            if (!resize_files_array(file_capacity * 2)) {
                FindClose(hFind);
                return 0;
            }
        }

        files[file_count].name = NULL;
        files[file_count].size = 0;
        files[file_count].is_directory = FALSE;

        int name_len = _tcslen(find_data.cFileName);
        files[file_count].name = (TCHAR*)malloc((name_len + 1) * sizeof(TCHAR));
        if (!files[file_count].name) {
            _tprintf(_T("Ошибка выделения памяти для имени файла!\n"));
            continue;
        }
        _tcscpy_s(files[file_count].name, name_len + 1, find_data.cFileName);

        files[file_count].size = file_size_to_bytes(&find_data);
        files[file_count].is_directory =
            (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
        files[file_count].creation_time = find_data.ftCreationTime;
        files[file_count].modify_time = find_data.ftLastWriteTime;

        file_count++;

    } while (FindNextFile(hFind, &find_data) != 0);

    DWORD error = GetLastError();
    if (error != ERROR_NO_MORE_FILES) {
        _tprintf(_T("Ошибка FindNextFile: %lu\n"), error);
        FindClose(hFind);
        return 0;
    }

    FindClose(hFind);
    return 1;
}

LONGLONG file_size_to_bytes(const WIN32_FIND_DATA* fd) {
    LARGE_INTEGER size;
    size.LowPart = fd->nFileSizeLow;
    size.HighPart = fd->nFileSizeHigh;
    return size.QuadPart;
}

TCHAR* format_size(LONGLONG bytes) {
    static TCHAR buffer[50];

    if (bytes < 1024) {
        _stprintf_s(buffer, 50, _T("%lld B"), bytes);
    }
    else if (bytes < 1024 * 1024) {
        _stprintf_s(buffer, 50, _T("%.2f KB"), bytes / 1024.0);
    }
    else if (bytes < 1024 * 1024 * 1024) {
        _stprintf_s(buffer, 50, _T("%.2f MB"), bytes / (1024.0 * 1024.0));
    }
    else {
        _stprintf_s(buffer, 50, _T("%.2f GB"),
            bytes / (1024.0 * 1024.0 * 1024.0));
    }

    return buffer;
}

void print_files() {
    _tprintf(_T("%-5s %-40s %-12s %-10s %-20s\n"),
        _T("№"), _T("Имя файла"), _T("Размер"), _T("Тип"), _T("Дата изменения"));
    _tprintf(_T("%s\n"), _T("-----------------------------------------------------------------------"));

    for (int i = 0; i < file_count; i++) {
        if (!files[i].name) {
            continue;
        }

        TCHAR* size_str = format_size(files[i].size);

        SYSTEMTIME st;
        FileTimeToSystemTime(&files[i].modify_time, &st);
        TCHAR time_str[20];
        _stprintf_s(time_str, 20, _T("%02d.%02d.%04d"),
            st.wDay, st.wMonth, st.wYear);

        TCHAR display_name[41];
        _tcsncpy_s(display_name, 41, files[i].name, 40);
        display_name[40] = _T('\0');
        if (_tcslen(files[i].name) > 40) {
            display_name[37] = _T('.');
            display_name[38] = _T('.');
            display_name[39] = _T('.');
        }

        _tprintf(_T("%-5d %-40s %-12s %-10s %-20s\n"),
            i + 1,
            display_name,
            files[i].is_directory ? _T("<DIR>") : size_str,
            files[i].is_directory ? _T("Folder") : _T("File"),
            time_str);
    }
    _tprintf(_T("\n"));
}

void simple_sort(FileInfo* arr, int n, BOOL ascending) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            BOOL should_swap = FALSE;

            if (ascending) {
                should_swap = arr[i].size > arr[j].size;
            }
            else {
                should_swap = arr[i].size < arr[j].size;
            }

            if (should_swap) {
                safe_swap_files(&arr[i], &arr[j]);
            }
        }
    }
}

void selection_sort(FileInfo* arr, int n, BOOL ascending) {
    for (int i = 0; i < n - 1; i++) {
        int extreme_idx = i;

        for (int j = i + 1; j < n; j++) {
            if (ascending) {
                if (arr[j].size < arr[extreme_idx].size) {
                    extreme_idx = j;
                }
            }
            else {
                if (arr[j].size > arr[extreme_idx].size) {
                    extreme_idx = j;
                }
            }
        }

        if (extreme_idx != i) {
            safe_swap_files(&arr[i], &arr[extreme_idx]);
        }
    }
}

void insertion_sort(FileInfo* arr, int n, BOOL ascending) {
    for (int i = 1; i < n; i++) {
        FileInfo key;
        key.name = NULL;
        deep_copy_fileinfo(&key, &arr[i]);

        int j = i - 1;

        while (j >= 0 &&
            ((ascending && arr[j].size > key.size) ||
                (!ascending && arr[j].size < key.size))) {
            if (arr[j + 1].name) {
                free(arr[j + 1].name);
            }
            arr[j + 1] = arr[j];
            j--;
        }

        if (arr[j + 1].name) {
            free(arr[j + 1].name);
        }
        arr[j + 1] = key;
    }
}

void merge(FileInfo* arr, int left, int mid, int right, BOOL ascending) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    FileInfo* L = (FileInfo*)calloc(n1, sizeof(FileInfo));
    FileInfo* R = (FileInfo*)calloc(n2, sizeof(FileInfo));

    if (!L || !R) {
        _tprintf(_T("Ошибка выделения памяти для слияния!\n"));
        if (L) free(L);
        if (R) free(R);
        return;
    }

    for (int i = 0; i < n1; i++) {
        deep_copy_fileinfo(&L[i], &arr[left + i]);
    }
    for (int j = 0; j < n2; j++) {
        deep_copy_fileinfo(&R[j], &arr[mid + 1 + j]);
    }

    int i = 0, j = 0, k = left;

    while (i < n1 && j < n2) {
        if ((ascending && L[i].size <= R[j].size) ||
            (!ascending && L[i].size >= R[j].size)) {
            if (arr[k].name) free(arr[k].name);
            arr[k] = L[i];
            L[i].name = NULL;
            i++;
        }
        else {
            if (arr[k].name) free(arr[k].name);
            arr[k] = R[j];
            R[j].name = NULL;
            j++;
        }
        k++;
    }

    while (i < n1) {
        if (arr[k].name) free(arr[k].name);
        arr[k] = L[i];
        L[i].name = NULL;
        i++;
        k++;
    }

    while (j < n2) {
        if (arr[k].name) free(arr[k].name);
        arr[k] = R[j];
        R[j].name = NULL;
        j++;
        k++;
    }

    for (int i = 0; i < n1; i++) {
        if (L[i].name) free(L[i].name);
    }
    for (int j = 0; j < n2; j++) {
        if (R[j].name) free(R[j].name);
    }

    free(L);
    free(R);
}

void merge_sort(FileInfo* arr, int left, int right, BOOL ascending) {
    if (left < right) {
        int mid = left + (right - left) / 2;

        merge_sort(arr, left, mid, ascending);
        merge_sort(arr, mid + 1, right, ascending);
        merge(arr, left, mid, right, ascending);
    }
}

int partition(FileInfo* arr, int left, int right, BOOL ascending) {
    LONGLONG pivot = arr[right].size;
    int i = left - 1;

    for (int j = left; j < right; j++) {
        if ((ascending && arr[j].size <= pivot) ||
            (!ascending && arr[j].size >= pivot)) {
            i++;
            safe_swap_files(&arr[i], &arr[j]);
        }
    }

    safe_swap_files(&arr[i + 1], &arr[right]);
    return i + 1;
}

void quick_sort(FileInfo* arr, int left, int right, BOOL ascending) {
    if (left < right) {
        int pi = partition(arr, left, right, ascending);

        quick_sort(arr, left, pi - 1, ascending);
        quick_sort(arr, pi + 1, right, ascending);
    }
}

void clear_screen() {
    system("cls");
}

void print_welcome() {
    _tprintf(_T("========================================\n"));
    _tprintf(_T("     ФАЙЛОВЫЙ МЕНЕДЖЕР С СОРТИРОВКОЙ    \n"));
    _tprintf(_T("========================================\n"));
}

void print_sort_methods() {
    _tprintf(_T("\n=== ДОСТУПНЫЕ МЕТОДЫ СОРТИРОВКИ ===\n"));
    _tprintf(_T("1. Простейшая сортировка (обменом)\n"));
    _tprintf(_T("2. Сортировка выбором\n"));
    _tprintf(_T("3. Сортировка вставками\n"));
    _tprintf(_T("4. Сортировка слиянием\n"));
    _tprintf(_T("5. Быстрая сортировка\n"));
    _tprintf(_T("====================================\n"));
}