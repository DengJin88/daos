package io.daos.fs.hadoop;

import io.daos.dfs.DaosUns;
import io.daos.dfs.uns.Layout;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.nio.file.Files;

/**
 *
 */
public class DaosFileSystemIT {
  private static final Logger LOG = LoggerFactory.getLogger(DaosFileSystemIT.class);

  private static FileSystem fs;

  @BeforeClass
  public static void setup() throws IOException {
    System.out.println("@BeforeClass");
    fs = DaosFSFactory.getFS();
  }

  //every time test one
  @Test
  public void testInitialization() throws Exception{
    initializationTest("daos://192.168.2.1:2345/", "daos://192.168.2.1:2345");
    initializationTest("daos://192.168.2.1:2345/abc", "daos://192.168.2.1:2345");
    initializationTest("daos://192.168.2.1:2345/ae/", "daos://192.168.2.1:2345");
    initializationTest("daos://192.168.2.1:2345/ac/path", "daos://192.168.2.1:2345");
    initializationTest("daos://192.168.2.1:2345/ac", "daos://192.168.2.1:2345");
    initializationTest("daos://192.168.2.1:2345/ad_c/", "daos://192.168.2.1:2345");
    initializationTest("daos://192.168.2.1:2345/ac2/path", "daos://192.168.2.1:2345");
    initializationTest("daos://192.168.2.1:2345/c.3", "daos://192.168.2.1:2345");
    initializationTest("daos://192.168.2.1:2345/234/", "daos://192.168.2.1:2345");
  }

  @Test
  public void testServiceLoader() throws Exception {
    Configuration cfg = new Configuration();
    cfg.set(Constants.DAOS_POOL_UUID, DaosFSFactory.pooluuid);
    cfg.set(Constants.DAOS_CONTAINER_UUID, DaosFSFactory.contuuid);
    cfg.set(Constants.DAOS_POOL_SVC, DaosFSFactory.svc);
    FileSystem fileSystem = FileSystem.get(URI.create("daos://2345:567/"), cfg);
    Assert.assertTrue(fileSystem instanceof DaosFileSystem);
  }

  private void initializationTest(String initializationUri, String expectedUri) throws Exception{
    fs.initialize(URI.create(initializationUri), DaosHadoopTestUtils.getConfiguration());
    Assert.assertEquals(URI.create(expectedUri), fs.getUri());
  }

  @Test
  public void testNewDaosFileSystemFromUns() throws Exception {
    File file = Files.createTempDirectory("uns").toFile();
    try {
      String path = file.getAbsolutePath();
      String daosAttr = String.format(io.daos.dfs.Constants.DUNS_XATTR_FMT, Layout.POSIX.name(),
                    DaosFSFactory.getPoolUuid(), DaosFSFactory.getContUuid());
      DaosUns.setAppInfo(path, io.daos.dfs.Constants.DUNS_XATTR_NAME, daosAttr);
      DaosUns.setAppInfo(path, Constants.UNS_ATTR_NAME_HADOOP,
              Constants.DAOS_POOL_FLAGS + "=2:");
      URI uri = URI.create("daos://" + Constants.DAOS_AUTHORITY_UNS + path);
      FileSystem fs = FileSystem.get(uri, new Configuration());
      Assert.assertNotNull(fs);
    } finally {
      file.delete();
    }
  }

  @AfterClass
  public static void teardown() throws Exception {
    if (fs != null) {
      fs.close();
    }
  }
}
