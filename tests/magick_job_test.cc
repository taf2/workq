#include "testlib.h"

WQ::DB::MysqlAdaptor *adaptor = NULL;
static job_creator_t job_creator = NULL;
static job_initialize_t job_initialize = NULL;
static job_finalize_t job_finalize = NULL;


static void test_thumbnail_from_photo()
{
  int status = 0;
  // load a job from the db
  WQ::DB::Adaptor::Iterator *it = adaptor->query_with_result("select id, name, data from jobs where name='magick' and status='pending' and locked_queue_id=0 limit %d", 1);
  check(it);
  if( it ) {
    WQ::DB::Adaptor::Row *row = it->begin();
    check(row);
    if( row ) {
      int job_id = row->int_field("id");
      const char *job_name = row->str_field("name");

      WQ::Job *job = job_creator(job_id,job_name,row,adaptor); //new WQ::MagickJob(job_id, job_name, row);

      status = job->real_execute();

      delete job;
      //delete row;, deleted by the job
    }
    delete it;
  }

  check_equal(1, status);
  // the job ran, check that:
  //
  // the job reported a status of complete
  //
  // the photo-thumb.jpg was created
  check( g_file_test("tests/fixtures/photo-thumb.jpg", G_FILE_TEST_EXISTS) );
  //
  // the photo-thumb.jpg is a valid image
  //
  // the photo-thumb.jpg has the correct width/height

}

int main(int argc, char **argv)
{
  g_type_init();
  g_thread_init(NULL);
  // initialize adaptor table
  WQ::DB::Adaptor::initialize();
  // register mysql adaptor
  WQ::DB::Adaptor::register_adaptor("mysql", WQ::DB::MysqlAdaptor::create );
  // load settings
  WQ_TEST_INIT_SETTINGS();
  // load fixture data
  WQ::DB::MysqlAdaptor ma;
  initialize_adaptor_with_fixtures(ma);
  adaptor = &ma;
  GModule *module = g_module_open("modules/magick", G_MODULE_BIND_MASK);

  if( !module ) {
    g_error("%s: %s", "magick", g_module_error ());
    return 1;
  }

  if( !g_module_symbol(module, "creator", (gpointer*)&job_creator) ) {
    g_error("%s: %s", "magick", g_module_error ());
    return 1;
  }
  g_module_symbol(module, "initialize", (gpointer*)&job_initialize);
  g_module_symbol(module, "finalize", (gpointer*)&job_finalize);

  job_initialize(argv[0]);

  // send the work queue some test jobs
  run_tests("Magick Job Test:", test_thumbnail_from_photo,  NULL);

  adaptor = NULL;
  job_finalize();
  g_module_close(module);

  return 0;
}
